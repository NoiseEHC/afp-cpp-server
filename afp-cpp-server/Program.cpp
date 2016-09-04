#include "stdafx.h"
#include "Program.h"
#include "Config.h"
#include "Portfolio.h"

#include <iostream>
#include <conio.h>

using namespace std;

Program::Program() :
	_currentState(make_shared<GlobalState>(GlobalState{ {}, {}, 1 })),
	_fakeConnections([this](string const &stockId, double newBuyPrice, double newSellPrice) { ProcessUpdate(stockId, newBuyPrice, newSellPrice); })
{
}

void Program::Run()
{
	boost::asio::io_service::work work(_io);
	vector<thread> ioThreads;
	for (uint32_t i = 0; i < 2; ++i)
		ioThreads.emplace_back([this]() { _io.run(); });

	ReloadConfig();
	auto sub = make_shared<Subsciption>(_io);
	sub->SubscribedList.emplace_back(make_shared<Portfolio>(_io));
	_currentState->SubsciptionHash["VOD LN"] = sub;
	_fakeConnections.Subscribe("VOD LN");

	while (true) {
		cout << "Press R to reload config, Q to quit" << endl;
		int key = _getch();
		if (key == 'q')
			break;
		if (key == 'r')
			ReloadConfig();
	}

	Shutdown();

	_io.stop();
	for (auto &t : ioThreads)
		if (t.joinable())
			t.join();
}

void Program::ConfigChange(shared_ptr<XmlConfig> newConfig)
{
}

void Program::ReloadConfig()
{
	ConfigChange(make_shared<XmlConfig>(XmlConfig::LoadFromXml("config.xml")));
}

void Program::Shutdown()
{
	ConfigChange(make_shared<XmlConfig>());
}

void Program::ProcessUpdate(string const &stockId, double newSellPrice, double newBuyPrice)
{
	// In a real server application this is the code which receives the network packet or other physical event,
	// here this code is called from the single thread of the FakeMarketData.
	auto state = atomic_load_explicit(&_currentState, memory_order_acquire);

	auto subscription = state->SubsciptionHash.find(stockId); // note that this is not a concurrent dictionary
	if (subscription != state->SubsciptionHash.end()) {
		auto packet = make_shared<PriceUpdate>(move(state), stockId, Price(newSellPrice, newBuyPrice));
		subscription->second->ProcessPacket(packet);
	}
}
