#include "stdafx.h"
#include "Program.h"
#include "Config.h"
#include "Portfolio.h"

#include <iostream>
#include <conio.h>
#include <stdexcept>
#include <unordered_set>

using namespace std;

Program::Program() :
	_currentState(make_shared<GlobalState>(GlobalState{ {}, {} })),
	_fakeConnections([this](string const &stockId, double newBuyPrice, double newSellPrice) { ProcessUpdate(stockId, newBuyPrice, newSellPrice); })
{
}

void Program::Run()
{
	boost::asio::io_service::work work(_io);
	vector<thread> ioThreads;
	for (uint32_t i = 0; i < 2; ++i)
		ioThreads.emplace_back([this]() { _io.run(); });

	ReloadConfig();

	while (true) {
		cout << "Press R to reload config, Q to quit" << endl;
		int key = _getch();
		if (key == 'q')
			break;
		if (key == 'r')
			ReloadConfig();
	}

	Shutdown();

	_io.stop();
	for (auto &t : ioThreads)
		if (t.joinable())
			t.join();
}

void Program::ReloadConfig()
{
	ConfigChange(make_shared<XmlConfig>(XmlConfig::LoadFromXml("config.xml")));
}

void Program::Shutdown()
{
	ConfigChange(make_shared<XmlConfig>());
}

void Program::ProcessUpdate(string const &stockId, double newSellPrice, double newBuyPrice)
{
	// In a real server application this is the code which receives the network packet or other physical event,
	// here this code is called from the single thread of the FakeMarketData.
	auto state = atomic_load_explicit(&_currentState, memory_order_acquire);

	auto subscription = state->SubsciptionHash.find(stockId); // note that this is not a concurrent dictionary
	if (subscription != state->SubsciptionHash.end()) {
		auto packet = make_shared<PriceUpdate>(move(state), stockId, Price(newSellPrice, newBuyPrice));
		subscription->second->ProcessPacket(packet);
	}
}

bool IsPortfolioDifferent(PortfolioConfig const &newConfig, Portfolio const &oldState)
{
	auto difference = Categorize<bool, string, StockConfig, string>(
		true, newConfig.StockList, [](auto const &item) { return item.StockId; },
		true, oldState.GetStockIdList(), [](auto const &item) { return item; },
		[](auto const &config, auto const &stockId) { return config.StockId != stockId; });
	return any_of(cbegin(difference), cend(difference), [](auto const &item) { return item.Category; });
}

shared_ptr<Portfolio> Program::CreatePortfolioFromConfig(
	PortfolioConfig const &config,
	unordered_map<string, MarketDataConfig> const &marketDataHash)
{
	vector<Portfolio::StockConfigWithCurrency> stockList;
	for (auto const &stock : config.StockList) {
		auto md = marketDataHash.find(stock.StockId);
		if (md != marketDataHash.end()) {
			stockList.emplace_back(Portfolio::StockConfigWithCurrency{
				stock.StockId,
				stock.Count,
				md->second.Id,
			});
		}
	}
	return make_shared<Portfolio>(_io, config.Id, stockList);
}

vector<CategorizeResult<Program::PortfolioChange, PortfolioConfig, shared_ptr<Portfolio>>> Program::CalculateNewPortfolioChanges(
	XmlConfig const &newConfig)
{
	return Categorize<PortfolioChange, string, PortfolioConfig, shared_ptr<Portfolio>>(
		PortfolioChange::NeedsStart, newConfig.PortfolioList, [](auto const &item) { return item.Id; },
		PortfolioChange::NeedsStop, _currentState->PortfolioList, [](auto const &item) { return item->Id; },
		[](auto const &left, auto const &right) { return IsPortfolioDifferent(left, *right) ? PortfolioChange::NeedsRestart : PortfolioChange::Nothing; });
}

unordered_map<string, shared_ptr<Portfolio>> Program::CalculateNewPortfolioHash(
	vector<CategorizeResult<PortfolioChange, PortfolioConfig, shared_ptr<Portfolio>>> const &changes,
	unordered_map<string, MarketDataConfig> const &marketDataHash)
{
	unordered_map<string, shared_ptr<Portfolio>> result;

	for (auto const &wtd : changes) {

		switch (wtd.Category) {
		case PortfolioChange::Nothing:
			result[(*wtd.Right)->Id] = *wtd.Right; // copy shared_ptr<Portfolio>
			break;

		case PortfolioChange::NeedsStart:
			cout << "Starting portfolio " << wtd.Left->Id << " with " << wtd.Left->StockList.size() << " stocks " << endl;
		case PortfolioChange::NeedsRestart: // Note that restarting a porfolio "forgets" the current price.
			result[wtd.Left->Id] = CreatePortfolioFromConfig(*wtd.Left, marketDataHash);
			break;

		case PortfolioChange::NeedsStop:
			cout << "Stopping portfolio " << (*wtd.Right)->Id << endl;
			// just leave out the object from the new list
			break;
		}
	}

	return result;
}

static unordered_map<string, MarketDataConfig> CalculateMarketDataHash(XmlConfig const &newConfig)
{
	unordered_map<string, MarketDataConfig> result;
	for (auto const &item : newConfig.MarketDataList)
		result[item.Id] = item;
	return result;
}

vector<PortfolioConfig> Program::CalculateNewPortfolioConfigList(
	vector<CategorizeResult<PortfolioChange, PortfolioConfig, shared_ptr<Portfolio>>> const &changes)
{
	vector<PortfolioConfig> result;
	for (auto const &item : changes)
		if (item.Category != PortfolioChange::NeedsStop)
			result.emplace_back(*item.Left);
	return result;
}

vector<Program::MarketDataUsage> Program::GroupPortfolioBySubscription(
	unordered_map<string, MarketDataConfig> const &marketDataHash,
	vector<PortfolioConfig> const &newPortfolioConfigList)
{
	unordered_map<string, vector<string>> result;
	for (auto const &portfolio : newPortfolioConfigList) {
		unordered_set<string> allMarketDataId;
		for (auto const &stock : portfolio.StockList) {
			auto stockMarketData = marketDataHash.find(stock.StockId);
			if (stockMarketData == marketDataHash.cend())
				throw logic_error("Invalid stock id");
			if (stockMarketData->second.IsCurrency)
				throw logic_error("Stock should not be a currency");
			auto currencyMarketData = marketDataHash.find(stockMarketData->second.CurrencyId);
			if (currencyMarketData == marketDataHash.cend())
				throw logic_error("Invalid stock currency id");
			allMarketDataId.insert(stockMarketData->second.Id);
			allMarketDataId.insert(currencyMarketData->second.Id);
		}
		for (auto const &id : allMarketDataId)
			result[id].push_back(portfolio.Id);
	}
	vector<MarketDataUsage> returned;
	transform(cbegin(result), cend(result), back_inserter(returned), [](auto const &item) { return MarketDataUsage{ item.first, item.second }; });
	return returned;
}

vector<CategorizeResult<Program::SubscriptionChange, Program::MarketDataUsage, shared_ptr<Subsciption>>> Program::CalculateSubscribeChanges(
	vector<shared_ptr<Subsciption>> const &subsciptionList,
	vector<MarketDataUsage> const &portfolioIdBySubscriptionId)
{
	return Categorize<SubscriptionChange, string, MarketDataUsage, shared_ptr<Subsciption>>(
		SubscriptionChange::NeedsSubscribe, portfolioIdBySubscriptionId, [](auto const &item) { return item.Id; },
		SubscriptionChange::NeedsUnsubscribe, subsciptionList, [](auto const &item) { return item->Id; },
		[](auto const &, auto const &) { return SubscriptionChange::NeedsChange; }); // Note that it always returns change...
}

void Program::PerformConfigChange(
	shared_ptr<XmlConfig> newConfig,
	unordered_map<string, MarketDataConfig> const &marketDataHash,
	unordered_map<string, shared_ptr<Portfolio>> const &newPortfolioHash,
	vector<CategorizeResult<Program::SubscriptionChange, Program::MarketDataUsage, shared_ptr<Subsciption>>> const &subscribeChanges)
{
	unordered_map<string, shared_ptr<Subsciption>> newSubscriptionHash;
	for (auto const &s : subscribeChanges) {
		switch (s.Category) {
		case SubscriptionChange::NeedsSubscribe:
		{
			auto cfg = marketDataHash.find(s.Left->Id);
			if (cfg == marketDataHash.end())
				throw new logic_error("Invalid subscription id");
			auto sub = make_shared<Subsciption>(s.Left->Id);
			for (auto const &item : s.Left->UsedBy)
				sub->SubscribedList.emplace_back(newPortfolioHash.at(item));
			newSubscriptionHash[s.Left->Id] = sub;
			break;
		}
		case SubscriptionChange::NeedsUnsubscribe:
			// just leave out the object from newSubscriptionHash
			cerr << "Unsubscribing: " << (*s.Right)->Id << endl;
			_fakeConnections.Unsubscribe((*s.Right)->Id);
			break;
		case SubscriptionChange::NeedsChange:
		{
			// Note that this would require to work properly having a shared state & queue.
			auto copy = make_shared<Subsciption>(**s.Right);
			for (auto const &item : s.Left->UsedBy)
				copy->SubscribedList.emplace_back(newPortfolioHash.at(item));
			newSubscriptionHash[s.Left->Id] = copy;
		}
		}
	}

	auto newState = make_shared<GlobalState>(GlobalState{ {}, newSubscriptionHash });
	for (auto const &item : newPortfolioHash)
		newState->PortfolioList.emplace_back(item.second);
	atomic_store_explicit(&_currentState, newState, memory_order_release);

	// Note that everything which can fail from now on must be part of the "nonconfiguration" part of the program!!!

	for (auto const &s : subscribeChanges) {
		if (s.Category == SubscriptionChange::NeedsSubscribe) {
			cerr << "Subscribing: " << s.Left->Id << " with " << s.Left->UsedBy.size() << " portfolios " << endl;
			_fakeConnections.Subscribe(s.Left->Id);
		}
	}
}

void Program::ConfigChange(shared_ptr<XmlConfig> newConfig)
{
	auto marketDataHash = CalculateMarketDataHash(*newConfig);
	auto portfolioChanges = CalculateNewPortfolioChanges(*newConfig);
	auto newPortfolioHash = CalculateNewPortfolioHash(portfolioChanges, marketDataHash);
	auto newPortfolioConfigList = CalculateNewPortfolioConfigList(portfolioChanges);
	auto portfolioIdBySubscriptionId = GroupPortfolioBySubscription(marketDataHash, newPortfolioConfigList);
	vector<shared_ptr<Subsciption>> subsciptionList;
	for (auto const &item : _currentState->SubsciptionHash)
		subsciptionList.emplace_back(item.second);
	auto subscribeChanges = CalculateSubscribeChanges(subsciptionList, portfolioIdBySubscriptionId);

	PerformConfigChange(newConfig, marketDataHash, newPortfolioHash, subscribeChanges);
}
