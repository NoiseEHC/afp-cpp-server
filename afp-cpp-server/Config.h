#pragma once

struct StockConfig
{
	std::string StockId; //!< Points to a MarketDataConfig where IsCurrency == false.
	uint64_t Count; //!< Number of shares of this stock.
};

struct PortfolioConfig
{
	std::string Id; //!< Id of the portfolio.
	std::vector<StockConfig> StockList; //!< The stocks which make up the portfolio.
};

struct MarketDataConfig
{
	std::string Id; //!< Id of the stock or currency.
	bool IsCurrency; //!< If true then it can be used as a currency, otherwise as a stock.
	std::string CurrencyId; //!< If it is a stock, it uses this currency (ignored for currencies).
	std::string Username; //!< This is for imaginary marked data feed connections.
	std::string Password; //!< This is for imaginary marked data feed connections.
};

struct XmlConfig
{
	std::vector<PortfolioConfig> PortfolioList;
	std::vector<MarketDataConfig> MarketDataList;

	static XmlConfig LoadFromXml(char const *filename);
};
