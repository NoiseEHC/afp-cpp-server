#pragma once

#include "boost/asio.hpp"
#include "FakeMarketData.h"
#include "Categorize.h"

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

	enum class PortfolioChange
	{
		Nothing,
		NeedsStop,
		NeedsStart,
		NeedsRestart,
	};

	std::shared_ptr<Portfolio> CreatePortfolioFromConfig(
		PortfolioConfig const &config,
		std::unordered_map<std::string, MarketDataConfig> const &marketDataHash);
	std::vector<CategorizeResult<PortfolioChange, PortfolioConfig, std::shared_ptr<Portfolio>>> CalculateNewPortfolioChanges(
		XmlConfig const &newConfig);
	std::unordered_map<std::string, std::shared_ptr<Portfolio>> CalculateNewPortfolioHash(
		std::vector<CategorizeResult<PortfolioChange, PortfolioConfig, std::shared_ptr<Portfolio>>> const &changes,
		std::unordered_map<std::string, MarketDataConfig> const &marketDataHash);
	std::vector<PortfolioConfig> CalculateNewPortfolioConfigList(
		std::vector<CategorizeResult<PortfolioChange, PortfolioConfig, std::shared_ptr<Portfolio>>> const &changes);

public:
	Program();
	void Run();
};
