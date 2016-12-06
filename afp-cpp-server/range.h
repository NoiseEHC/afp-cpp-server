#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm_ext.hpp>

template<
	typename SinglePassRange,
	typename ValueType = typename boost::range_value<SinglePassRange>::type
>
auto to_vector(
	SinglePassRange const &from
)->std::vector<ValueType>
{
	return std::vector<ValueType>(std::begin(from), std::end(from));
}

template<
	typename SinglePassRange,
	typename ValueType = typename boost::range_value<SinglePassRange>::type
>
auto to_unordered_set(
	SinglePassRange const &from
)->std::unordered_set<ValueType>
{
	return std::unordered_set<ValueType>(std::begin(from), std::end(from));
}

template<
	typename SinglePassRange,
	typename GetKeyFunction,
	typename ValueType = typename boost::range_value<SinglePassRange>::type,
	typename KeyType = typename std::result_of<GetKeyFunction(ValueType const &)>::type
>
auto to_unordered_map(
	SinglePassRange const &from,
	GetKeyFunction getKey
)->std::unordered_map<KeyType, ValueType>
{
	auto items = from | transformed([=](auto const &item) { return std::make_pair(getKey(item), item); });
	return std::unordered_map<KeyType, ValueType>(std::begin(items), std::end(items));
}
