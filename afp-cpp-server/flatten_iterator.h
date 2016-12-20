#pragma once

#include <boost/iterator_adaptors.hpp>

namespace boost {
namespace iterators {

template<typename OuterIterator, typename InnerIterator>
class flatten_iterator : public boost::iterator_facade<
	flatten_iterator<OuterIterator, InnerIterator>,
	typename iterator_value<InnerIterator>::type,
	boost::single_pass_traversal_tag,
	typename iterator_reference<InnerIterator>::type>
{
	friend class boost::iterator_core_access;

	OuterIterator _outer_current;
	OuterIterator _outer_end;
	InnerIterator _inner_current;
	InnerIterator _inner_end;

	bool equal(flatten_iterator<OuterIterator, InnerIterator> const &other) const
	{
		return
			(this->_outer_current == this->_outer_end && other._outer_current == other._outer_end) ||
			(this->_outer_current == other._outer_current && this->_inner_current == other._inner_current);
	}

	typename iterator_facade_::reference dereference() const
	{
		return *_inner_current;
	}

	void increment()
	{
		++_inner_current;
		if (_inner_current == _inner_end) {
			++_outer_current;
			ensure_valid();
		}
	}

	void ensure_valid()
	{
		while (_outer_current != _outer_end) {
			_inner_current = boost::begin(*_outer_current);
			_inner_end = boost::end(*_outer_current);
			if (_inner_current != _inner_end)
				break;
			++_outer_current;
		}
	}

public:
	explicit flatten_iterator(OuterIterator begin, OuterIterator end) :
		_outer_current(begin),
		_outer_end(end)
	{
		ensure_valid();
	}

	explicit flatten_iterator(OuterIterator end) :
		_outer_current(end),
		_outer_end(end)
	{
	}
};

template<typename Collection>
auto make_begin(Collection &collection) -> flatten_iterator<decltype(boost::begin(collection)), decltype(boost::begin(*boost::begin(collection)))>
{
	return flatten_iterator<decltype(boost::begin(collection)), decltype(boost::begin(*boost::begin(collection)))>(boost::begin(collection), boost::end(collection));
}

template<typename Collection>
auto make_end(Collection &collection) -> flatten_iterator<decltype(boost::begin(collection)), decltype(boost::begin(*boost::begin(collection)))>
{
	return flatten_iterator<decltype(boost::begin(collection)), decltype(boost::begin(*boost::begin(collection)))>(boost::end(collection));
}

}
}
