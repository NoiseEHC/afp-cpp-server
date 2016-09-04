#pragma once

#include "Price.h"
#include "Config.h"

#include "boost/asio.hpp"

struct PriceUpdate;

class Portfolio
{
	boost::asio::io_service::strand _strand;
	std::unordered_map<std::string, Price> _lastStockPrice;
	int _called;

public:
	std::string Id;

	Portfolio(boost::asio::io_service &io, std::string const &id, std::vector<std::string> const &stockList);

	template <typename CompletionHandler>
	BOOST_ASIO_INITFN_RESULT_TYPE(CompletionHandler, void()) Enque(BOOST_ASIO_MOVE_ARG(CompletionHandler) handler) {
		return _strand.post(BOOST_ASIO_MOVE_CAST(CompletionHandler)(handler));
	}

	void UpdateStockPrice(std::string const &stockId, Price const &newPrice);
	std::vector<std::string> GetStockIdList() const;

	void IncrementCalled(int increment) {
		_called += increment;
	}
};

class Subsciption
{
	boost::asio::io_service::strand _strand;
	Price _lastPrice;

public:
	std::vector<std::shared_ptr<Portfolio>> SubscribedList;

	Subsciption(boost::asio::io_service &io);
	void ProcessPacket(std::shared_ptr<PriceUpdate> const &packet);
};

struct GlobalState
{
	std::vector<std::shared_ptr<Portfolio>> PortfolioList;
	std::unordered_map<std::string, std::shared_ptr<Subsciption>> SubsciptionHash;
	int Increment;
};

struct PriceUpdate
{
	std::shared_ptr<GlobalState> State;
	std::string StockId;
	Price NewPrice;

	PriceUpdate(std::shared_ptr<GlobalState> &&state, std::string const &stockId, Price const &newPrice);
};
