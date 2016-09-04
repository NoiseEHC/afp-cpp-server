#pragma once

#include "boost/asio.hpp"
#include "FakeMarketData.h"

struct GlobalState;
class Portfolio;

struct XmlConfig;
struct PortfolioConfig;
struct MarketDataConfig;

class Program
{
	boost::asio::io_service _io;
	std::shared_ptr<GlobalState> _currentState;
	FakeMarketData _fakeConnections;

	void ConfigChange(std::shared_ptr<XmlConfig> newConfig);
	void ReloadConfig();
	void Shutdown();
	void ProcessUpdate(std::string const &stockId, double newSellPrice, double newBuyPrice);
	std::shared_ptr<Portfolio> CreatePortfolioFromConfig(
		PortfolioConfig const &config,
		std::unordered_map<std::string, std::shared_ptr<MarketDataConfig>> const &marketDataHash);

public:
	Program();
	void Run();
};
