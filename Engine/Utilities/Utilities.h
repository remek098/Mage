#pragma once

#define USE_STL_VECTOR 0
#define USE_STL_DEQUE 1

#if USE_STL_VECTOR
#include <vector>
#include <algorithm>


namespace mage::utl {

	template<typename T>
	using vector = std::vector<T>;

	// swap element at index with element at the end.
	// if there's only 1 element in vector or vector is empty, clear array
	template<typename T>
	void erase_unordered(T& v, size_t index) {
		if ( v.size() > 1 ) {
			std::iter_swap(v.begin() + index, v.end() - 1);
			v.pop_back();
		}
		else {
			v.clear();
		}
	}
}

#else
#include "Vector.h"
namespace mage::utl {
	/// <summary>
	/// Any class that has a function called erase_unordered(u64) can be used as T.
	/// </summary>
	/// <typeparam name="T"></typeparam>
	/// <param name="v">assuming the type you can give it is including the typeof vector.</param>
	/// <param name="index"></param>
	template<typename T>
	void erase_unordered(T& v, u64 index) {
		v.erase_unordered(index);
	}
}
#endif

#if USE_STL_DEQUE
#include <deque>
namespace mage::utl {

	template<typename T>
	using deque = std::deque<T>;
}
#endif

namespace mage::utl {

	// TODO: implement our own containers
}

#include "FreeList.h"