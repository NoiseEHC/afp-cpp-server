#pragma once

#include "stdafx.h"
#include <limits>

struct Price {
	double BuyPrice;
	double SellPrice;

	Price(double buyPrice, double sellPrice) :
		BuyPrice(buyPrice),
		SellPrice(sellPrice)
	{
	}

	Price operator +(Price const &other)
	{
		return Price(BuyPrice + other.BuyPrice, SellPrice + other.SellPrice);
	}

	Price operator *(double const &other)
	{
		return Price(BuyPrice * other, SellPrice * other);
	}
};

struct PriceUpdate
{
	std::string StockId;
	Price NewPrice;

	PriceUpdate() :
		StockId(),
		NewPrice(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN())
	{
	}

	PriceUpdate(std::string const &stockId, Price const &newPrice) :
		StockId(stockId),
		NewPrice(newPrice)
	{
	}
};
