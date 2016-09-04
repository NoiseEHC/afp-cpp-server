#pragma once

#include <vector>

template<typename C, typename T1, typename T2>
struct CategorizeResult
{
	C Category;
	T1 const *Left;
	T2 const *Right;
};

template<typename C, typename K, typename T1, typename T2>
std::vector<CategorizeResult<C, T1, T2>> Categorize(
	C leftOnly, std::vector<T1> const &leftList, std::function<K(T1 const &)> getLeftKey,
	C rightOnly, std::vector<T2> const &rightList, std::function<K(T2 const &)> getRightKey,
	std::function<C(T1 const &, T2 const &)> getCategory)
{
	std::vector<CategorizeResult<C, T1, T2>> result;
	unordered_map<K, T1 const *> leftHash;
	for (auto const &item : leftList)
		leftHash[getLeftKey(item)] = &item;
	unordered_map<K, T2 const *> rightHash;
	for (auto const &item : rightList)
		rightHash[getRightKey(item)] = &item;
	for (auto const &kv : leftHash) {
		auto rightIterator = rightHash.find(kv.first);
		if (rightIterator != rightHash.end()) {
			result.emplace_back(CategorizeResult<C, T1, T2>{ getCategory(*kv.second, *(rightIterator->second)), kv.second, rightIterator->second });
		}
		else {
			result.emplace_back(CategorizeResult<C, T1, T2>{ leftOnly, kv.second, nullptr });
		}
	}
	for (auto const &kv : rightHash) {
		auto leftIterator = leftHash.find(kv.first);
		if (leftIterator == leftHash.end()) {
			result.emplace_back(CategorizeResult<C, T1, T2>{ rightOnly, nullptr, kv.second });
		}
	}
	return result;
}
