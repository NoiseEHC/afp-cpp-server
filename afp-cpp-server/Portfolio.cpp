#include "stdafx.h"
#include "Portfolio.h"
#include <iostream>

using namespace std;

void Portfolio::Recalculate(string const &lastChangedId) const
{
	Price result = Price{ 0.0, 0.0 };
	for (auto const &stock : _stockList) {
		auto const &currency = _lastStockPrice.at(stock.CurrencyId);
		double midCurrency = (currency.BuyPrice + currency.SellPrice) / 2;
		result = result + _lastStockPrice.at(stock.StockId) * midCurrency * (double)stock.Count;
	}
	auto _ = lastChangedId;
	//cout << Id << "(" << lastChangedId << ") == " << result.BuyPrice << "," << result.SellPrice << endl;
}

Portfolio::Portfolio(boost::asio::io_service & io, string const &id, vector<StockConfigWithCurrency> const &stockList) :
	_strand(io),
	Id(id),
	_stockList(stockList)
{
	for (auto const &item : _stockList) {
		_lastStockPrice.emplace(make_pair(item.StockId, Price(numeric_limits<double>::quiet_NaN(), numeric_limits<double>::quiet_NaN())));
		_lastStockPrice.emplace(make_pair(item.CurrencyId, Price(numeric_limits<double>::quiet_NaN(), numeric_limits<double>::quiet_NaN())));
	}
}

vector<string> Portfolio::GetStockIdList() const
{
	vector<string> result;
	transform(cbegin(_lastStockPrice), cend(_lastStockPrice), back_inserter(result), [](auto const &item) { return item.first; });
	return result;
}

void Portfolio::ProcessPacket(shared_ptr<PriceUpdate> const & packet)
{
	_lastStockPrice.insert_or_assign(packet->StockId, packet->NewPrice);
	Recalculate(packet->StockId);
}

Subscription::Subscription(string const &id) :
	_lastPrice(0.0, 0.0),
	Id(id)
{
}

void Subscription::ProcessPacket(shared_ptr<PriceUpdate> const &packet)
{
	// Here comes the code which must be executed synchronously during packet processing. (eg must be executed once per Subscription)

	// We filter the incoming prices (that is the synchronous processing), and if the price has been changed, distribute the event.
	if (_lastPrice == packet->NewPrice)
		return;
	_lastPrice = packet->NewPrice;

	for (auto const &p : SubscribedList)
		p->Enque([p, packet]() { p->ProcessPacket(packet); });
}

PriceUpdate::PriceUpdate(shared_ptr<GlobalState>&& state, string const & stockId, Price const & newPrice) :
	State(move(state)),
	StockId(stockId),
	NewPrice(newPrice)
{
}
