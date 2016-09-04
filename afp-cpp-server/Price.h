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

	Price operator +(Price const &other) const
	{
		return Price(BuyPrice + other.BuyPrice, SellPrice + other.SellPrice);
	}

	Price operator *(double const &other) const
	{
		return Price(BuyPrice * other, SellPrice * other);
	}

	bool operator ==(Price const &other) const
	{
		return BuyPrice == other.BuyPrice && SellPrice == other.SellPrice;
	}

	bool operator !=(Price const &other) const
	{
		return !(*this == other);
	}
};
