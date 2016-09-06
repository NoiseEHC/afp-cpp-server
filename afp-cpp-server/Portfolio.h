#pragma once

#include "Price.h"
#include "Config.h"

#include "boost/asio.hpp"

struct PriceUpdate;

class Portfolio
{
public:
	struct StockConfigWithCurrency
	{
		std::string StockId; //!< Points to a MarketDataConfig where IsCurrency == false.
		uint64_t Count; //!< Number of shares of this stock.
		std::string CurrencyId; //!< Points to a MarketDataConfig where IsCurrency == true.
	};

private:
	boost::asio::io_service::strand _strand;
	std::unordered_map<std::string, Price> _lastStockPrice;
	std::vector<StockConfigWithCurrency> _stockList;
	void Recalculate(std::string const &lastChangedId) const;

public:
	std::string Id;

	Portfolio(boost::asio::io_service &io, std::string const &id, std::vector<StockConfigWithCurrency> const &stockList);

	template <typename CompletionHandler>
	BOOST_ASIO_INITFN_RESULT_TYPE(CompletionHandler, void()) Enque(BOOST_ASIO_MOVE_ARG(CompletionHandler) handler) {
		return _strand.post(BOOST_ASIO_MOVE_CAST(CompletionHandler)(handler));
	}

	std::vector<std::string> GetStockIdList() const;

	void ProcessPacket(std::shared_ptr<PriceUpdate> const &packet);
};

class Subsciption
{
	Price _lastPrice;

public:
	std::string Id;
	std::vector<std::shared_ptr<Portfolio>> SubscribedList;

	Subsciption(std::string const &id);
	void ProcessPacket(std::shared_ptr<PriceUpdate> const &packet);
};

struct GlobalState
{
	std::vector<std::shared_ptr<Portfolio>> PortfolioList;
	std::unordered_map<std::string, std::shared_ptr<Subsciption>> SubsciptionHash;
};

struct PriceUpdate
{
	std::shared_ptr<GlobalState> State;
	std::string StockId;
	Price NewPrice;

	PriceUpdate(std::shared_ptr<GlobalState> &&state, std::string const &stockId, Price const &newPrice);
};
