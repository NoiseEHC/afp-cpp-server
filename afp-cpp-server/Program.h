#pragma once

#include "boost/asio.hpp"
#include "FakeMarketData.h"

struct GlobalState;
struct XmlConfig;

class Program
{
	boost::asio::io_service _io;
	std::shared_ptr<GlobalState> _currentState;
	FakeMarketData _fakeConnections;

	void ConfigChange(std::shared_ptr<XmlConfig> newConfig);
	void ReloadConfig();
	void Shutdown();
	void ProcessUpdate(std::string const &stockId, double newSellPrice, double newBuyPrice);

public:
	Program();
	void Run();
};
