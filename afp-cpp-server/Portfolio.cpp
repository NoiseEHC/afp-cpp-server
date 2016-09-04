#include "stdafx.h"
#include "Portfolio.h"

using namespace std;

Portfolio::Portfolio(boost::asio::io_service & io, string const &id, vector<string> const &stockList) :
	_strand(io),
	_called(0),
	Id(id)
{
	for (auto const &item : stockList)
		_lastStockPrice.emplace(make_pair(item, Price(numeric_limits<double>::quiet_NaN(), numeric_limits<double>::quiet_NaN())));
}

void Portfolio::UpdateStockPrice(string const & stockId, Price const & newPrice)
{
	auto stock = _lastStockPrice.find(stockId);
	if (stock != _lastStockPrice.end()) {
		stock->second = newPrice;
		//TODO: recalculate
	}
}

vector<string> Portfolio::GetStockIdList() const
{
	vector<string> result;
	transform(cbegin(_lastStockPrice), cend(_lastStockPrice), back_inserter(result), [](auto const &item) { return item.first; });
	return result;
}

Subsciption::Subsciption(boost::asio::io_service &io) :
	_strand(io),
	_lastPrice(0.0, 0.0)
{
}

void Subsciption::ProcessPacket(shared_ptr<PriceUpdate> const &packet)
{
	// Here comes the code which must be executed synchronously during packet processing. (eg must be executed once per Subscription)

	// We filter the incoming prices (that is the synchronous processing), and if the price has been changed, distribute the event.
	if (_lastPrice == packet->NewPrice)
		return;
	_lastPrice = packet->NewPrice;

	for (auto const &p : SubscribedList)
		p->Enque([p, packet]() { p->IncrementCalled(packet->State->Increment); });
}

PriceUpdate::PriceUpdate(shared_ptr<GlobalState>&& state, string const & stockId, Price const & newPrice) :
	State(move(state)),
	StockId(stockId),
	NewPrice(newPrice)
{
}
