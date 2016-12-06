#include "stdafx.h"
#include "Program.h"
#include "Config.h"
#include "Portfolio.h"

#include <iostream>
#include <conio.h>
#include <stdexcept>
#include <unordered_set>
#include "Range.h"

using namespace std;
using namespace boost::adaptors;

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

void Program::ProcessUpdate(string const &stockId, double newBuyPrice, double newSellPrice)
{
	// In a real server application this is the code which receives the network packet or other physical event,
	// here this code is called from the single thread of the FakeMarketData.
	auto state = atomic_load_explicit(&_currentState, memory_order_acquire);

	auto subscription = state->SubscriptionHash.find(stockId); //NOTE: this is not a concurrent dictionary, we do not need that.
	if (subscription != state->SubscriptionHash.end()) {
		auto packet = make_shared<PriceUpdate>(move(state), stockId, Price(newBuyPrice, newSellPrice));
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

		case PortfolioChange::NeedsRestart: //NOTE: restarting a porfolio "forgets" the current price.
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

static auto CalculateMarketDataHash(
	XmlConfig const &newConfig
)->unordered_map<string, MarketDataConfig>
{
	return to_unordered_map(newConfig.MarketDataList, 
		[](auto const &item) { return item.Id; });
}

auto Program::CalculateNewPortfolioConfigList(
	vector<CategorizeResult<PortfolioChange, PortfolioConfig, shared_ptr<Portfolio>>> const &changes
)->vector<PortfolioConfig>
{
	return to_vector(changes
		| filtered([](auto const &item) { return item.Category != PortfolioChange::NeedsStop; })
		| transformed([](auto const &item) { return *item.Left; }));
}

auto Program::GroupPortfolioBySubscription(
	unordered_map<string, MarketDataConfig> const &marketDataHash,
	vector<PortfolioConfig> const &newPortfolioConfigList
)->vector<MarketDataUsage>
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
	return to_vector(result 
		| transformed([](auto const &item) { return MarketDataUsage{ item.first, item.second }; }));
}

vector<CategorizeResult<Program::SubscriptionChange, Program::MarketDataUsage, shared_ptr<Subscription>>> Program::CalculateSubscribeChanges(
	vector<shared_ptr<Subscription>> const &subscriptionList,
	vector<MarketDataUsage> const &portfolioIdBySubscriptionId)
{
	return Categorize<SubscriptionChange, string, MarketDataUsage, shared_ptr<Subscription>>(
		SubscriptionChange::NeedsSubscribe, portfolioIdBySubscriptionId, [](auto const &item) { return item.Id; },
		SubscriptionChange::NeedsUnsubscribe, subscriptionList, [](auto const &item) { return item->Id; },
		[](auto const &, auto const &) { return SubscriptionChange::NeedsChange; }); //NOTE: it always returns changed, even if nothing changed...
}

void Program::PerformConfigChange(
	shared_ptr<XmlConfig> newConfig,
	unordered_map<string, MarketDataConfig> const &marketDataHash,
	unordered_map<string, shared_ptr<Portfolio>> const &newPortfolioHash,
	vector<CategorizeResult<Program::SubscriptionChange, Program::MarketDataUsage, shared_ptr<Subscription>>> const &subscribeChanges)
{
	//NOTE: we assume that nothing will fail from now on (so all memory must be preallocated), 
	// or if it fails does not matter (if unsubscribing fails, we just close the socket anyways),
	// otherwise that failing thing must be part of the "nonconfiguration" part of the program!!!

	unordered_map<string, shared_ptr<Subscription>> newSubscriptionHash;
	for (auto const &s : subscribeChanges) {
		switch (s.Category) {
		case SubscriptionChange::NeedsSubscribe:
		{
			auto cfg = marketDataHash.find(s.Left->Id);
			if (cfg == marketDataHash.end())
				throw new logic_error("Invalid subscription id");
			auto sub = make_shared<Subscription>(s.Left->Id);
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
			//NOTE: in order to work properly (not forgetting current price), it should have a shared state & queue.
			auto copy = make_shared<Subscription>(**s.Right);
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

	//NOTE: everything which can fail from now on, must be part of the "nonconfiguration" part of the program!!!

	for (auto const &s : subscribeChanges) {
		if (s.Category == SubscriptionChange::NeedsSubscribe) {
			cerr << "Subscribing: " << s.Left->Id << " with " << s.Left->UsedBy.size() << " portfolios " << endl;
			_fakeConnections.Subscribe(s.Left->Id);
		}
	}
}

void Program::ConfigChange(shared_ptr<XmlConfig> newConfig)
{
	//NOTE: it does not matter how slow the following code is, as it runs asynchronously to the "nonconfiguration" part of the program.

	auto marketDataHash = CalculateMarketDataHash(*newConfig);
	auto portfolioChanges = CalculateNewPortfolioChanges(*newConfig);
	auto newPortfolioHash = CalculateNewPortfolioHash(portfolioChanges, marketDataHash);
	auto newPortfolioConfigList = CalculateNewPortfolioConfigList(portfolioChanges);
	auto portfolioIdBySubscriptionId = GroupPortfolioBySubscription(marketDataHash, newPortfolioConfigList);
	auto subscriptionList = to_vector(_currentState->SubscriptionHash | map_values);
	auto subscribeChanges = CalculateSubscribeChanges(subscriptionList, portfolioIdBySubscriptionId);

	//NOTE: up to this point the code can just fail and it will make no changes to objects used by the 
	// "nonconfiguration" part of the program, so we just do not have to do anything to roll back changes.

	PerformConfigChange(newConfig, marketDataHash, newPortfolioHash, subscribeChanges);
}
