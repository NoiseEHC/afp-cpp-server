#pragma once

#include <boost/range/iterator_range.hpp>
#include <boost/iterator/reverse_iterator.hpp>

template< 
	typename SinglePassRange,
	typename SubRangeType = typename boost::range_value<SinglePassRange>::type,
	typename ValueType = typename boost::range_value<SubRange>::type
>
struct flattened_range :
	boost::iterator_range<typename boost::range_iterator<SubRangeType>::type>
{
private:
	typedef boost::iterator_range<typename boost::range_iterator<SubRangeType>::type>::type> base;

public:
	typedef typename boost::range_iterator<typename boost::range_iterator<R>::type>::type iterator;

	flattened_range(R& r)
		: base(r)
	{ }
};
