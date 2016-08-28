#include "stdafx.h"
#include "Config.h"
#include <iostream>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/numeric.hpp>
#include "../3rdParty/pugixml/src/pugixml.hpp"

using namespace std;
using namespace boost::adaptors;

XmlConfig XmlConfig::LoadFromXml(char const *filename)
{
	XmlConfig result;

	pugi::xml_document doc;
	auto root = doc.load_file(filename);
	if (root) {
		for (auto portfolio : doc.child("config").select_nodes("portfolio")) {
			auto p = PortfolioConfig{
				portfolio.node().attribute("id").value()
			};
			for (auto stock : portfolio.node().select_nodes("stock")) {
				p.StockList.emplace_back(StockConfig{
					stock.node().attribute("stockid").value(),
					stock.node().attribute("count").as_ullong(),
				});
			}
			result.PortfolioList.emplace_back(move(p));
		}
		for (auto marketData : doc.child("config").select_nodes("marketdata")) {
			result.MarketDataList.emplace_back(MarketDataConfig{
				marketData.node().attribute("id").value(),
				marketData.node().attribute("iscurrency").as_bool(),
				marketData.node().attribute("currencyid").value(),
				marketData.node().attribute("username").value(),
				marketData.node().attribute("password").value(),
			});
		}
	}

	cout << "Loaded config, number of portfolios: " << result.PortfolioList.size();
	cout << ", number of market data: " << result.MarketDataList.size();
	cout << ", number of stock references: " <<
		boost::accumulate(
			result.PortfolioList | transformed([](auto const &p) { return p.StockList.size(); }),
			0u);
	cout << endl;

	return result;
}

#include "../3rdParty/pugixml/src/pugixml.cpp"
