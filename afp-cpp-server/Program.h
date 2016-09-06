#pragma once

#include "boost/asio.hpp"
#include "FakeMarketData.h"
#include "Categorize.h"

struct GlobalState;
class Portfolio;
class Subsciption;

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

	enum class SubscriptionChange
	{
		NeedsSubscribe,
		NeedsUnsubscribe,
		NeedsChange,
	};

	struct MarketDataUsage
	{
		std::string Id;
		std::vector<std::string> UsedBy;
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
	std::vector<MarketDataUsage> GroupPortfolioBySubscription(
		std::unordered_map<std::string, MarketDataConfig> const &marketDataHash,
		std::vector<PortfolioConfig> const &newPortfolioConfigList);
	std::vector<CategorizeResult<SubscriptionChange, MarketDataUsage, std::shared_ptr<Subsciption>>> CalculateSubscribeChanges(
		std::vector<std::shared_ptr<Subsciption>> const &subsciptionList,
		std::vector<MarketDataUsage> const &portfolioIdBySubscriptionId);
	void Program::PerformConfigChange(
		std::shared_ptr<XmlConfig> newConfig,
		std::unordered_map<std::string, MarketDataConfig> const &marketDataHash,
		std::unordered_map<std::string, std::shared_ptr<Portfolio>> const &newPortfolioHash,
		std::vector<CategorizeResult<Program::SubscriptionChange, Program::MarketDataUsage, std::shared_ptr<Subsciption>>> const &subscribeChanges);

public:
	Program();
	void Run();
};
