#pragma once

#include "Price.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////////
/// This class keeps a list of active fake market data "connections" which on a distinct thread 
/// generate random price fluctuations. The thread passes the price update to the supplied callback.
////////////////////////////////////////////////////////////////////////////////////////////////////

class FakeMarketData
{
	class RandomPrice
	{
		std::string _stockId;
		boost::random::mt19937 _gen;
		double _sellPrice;
		double _buyPrice;

	public:
		RandomPrice(std::string stockId);
		std::string const &GetStockId() const { return _stockId; }
		bool UpdatePriceRandomly();
		PriceUpdate GetUpdate() const;
	};

	mutable std::mutex _fakeConnectionsLock;
	std::vector<RandomPrice> _fakeConnections;
	std::function<void(PriceUpdate const &)> _callback;
	std::atomic<bool> _stop;
	std::thread _thread;

	void ThreadProc();

public:
	FakeMarketData(std::function<void(PriceUpdate const &)> callback);
	~FakeMarketData();
	void Subscribe(std::string const &stockId);
	void Unsubscribe(std::string const &stockId);
};
