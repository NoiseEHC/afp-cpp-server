#include "stdafx.h"
#include "FakeMarketData.h"
#include <chrono>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
// FakeMarketData::RandomPrice
////////////////////////////////////////////////////////////////////////////////////////////////////

FakeMarketData::RandomPrice::RandomPrice(string stockId) :
	_stockId(stockId)
{
	_buyPrice = 50.0;
	_sellPrice = 51.0;
}

bool FakeMarketData::RandomPrice::UpdatePriceRandomly()
{
	boost::random::uniform_int_distribution<> dist(-2, 2);
	switch (dist(_gen)) {
	case -1:
		_buyPrice = max(_buyPrice - 0.1, 10.0);
		return true;
	case 1:
		_buyPrice = min(_buyPrice + 0.1, _sellPrice);
		return true;
	case -2:
		_sellPrice = max(_sellPrice - 0.1, _buyPrice);
		return true;
	case 2:
		_sellPrice = min(_sellPrice + 0.1, 100.0);
		return true;
	default: // 0 means do nothing
		return false;
	}
}

void FakeMarketData::RandomPrice::GetUpdate(string const* &stockId, double &newBuyPrice, double &newSellPrice) const
{
	stockId = &_stockId;
	newBuyPrice = _buyPrice;
	newSellPrice = _sellPrice;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// FakeMarketData
////////////////////////////////////////////////////////////////////////////////////////////////////

FakeMarketData::FakeMarketData(function<void(string const &stockId, double newBuyPrice, double newSellPrice)> callback) :
	_callback(callback),
	_stop(false),
	_thread(&FakeMarketData::ThreadProc, this)
{
}

FakeMarketData::~FakeMarketData()
{
	_stop = true;
	if (_thread.joinable())
		_thread.join();
}

void FakeMarketData::Subscribe(string const &stockId)
{
	lock_guard<mutex> lock(_fakeConnectionsLock);
	_fakeConnections.emplace_back(stockId);
}

void FakeMarketData::Unsubscribe(string const &stockId)
{
	lock_guard<mutex> lock(_fakeConnectionsLock);
	remove_if(begin(_fakeConnections), end(_fakeConnections), [&stockId](RandomPrice const &item) { return item.GetStockId() == stockId; });
}

void FakeMarketData::ThreadProc()
{
	boost::random::mt19937 gen;
	while (!_stop) {
		string const *stockId = nullptr;
		double newBuyPrice = 0.0;
		double newSellPrice = 0.0;
		bool thereIsAnUpdate = false;
		{
			lock_guard<mutex> lock(_fakeConnectionsLock);
			if (_fakeConnections.size() >= 1) {
				boost::random::uniform_int_distribution<> dist(0, _fakeConnections.size() - 1);
				auto &stock = _fakeConnections[dist(gen)];
				thereIsAnUpdate = stock.UpdatePriceRandomly();
				if (thereIsAnUpdate)
					stock.GetUpdate(stockId, newBuyPrice, newSellPrice);
			}
		}
		if (thereIsAnUpdate)
			_callback(*stockId, newBuyPrice, newSellPrice);
		using namespace std::chrono_literals;
		this_thread::sleep_for(100ms);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
