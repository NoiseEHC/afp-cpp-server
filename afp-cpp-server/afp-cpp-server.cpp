#include "stdafx.h"
#include "Config.h"
#include "FakeMarketData.h"
#include <iostream>

using namespace std;

int main()
{
	FakeMarketData fakeConnections([](PriceUpdate const &) {
		cout << "tick" << endl;
	});

	auto p = XmlConfig::LoadFromXml("config.xml");
	for (auto const &marketData : p.MarketDataList)
		fakeConnections.Subscribe(marketData.Id);

	this_thread::sleep_for(1000ms);

	return 0;
}
