#pragma once
#include "CommonHeaders.h"
//#include <limits>

namespace mage::id {
	/*
	https://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html
	*/
	using id_type = u32;

	// index part gives us the index of the entity in lookup array.
	// generation part is used to distinguish entities created at the same index slot
	
	constexpr u32 generation_bits{ 8 };
	constexpr u32 index_bits{ sizeof(id_type) * 8 - generation_bits};

	// mask to be able to retrieve information
	constexpr id_type generation_mask{ (id_type{1} << generation_bits) - 1 };
	constexpr id_type index_mask{ (id_type{1} << index_bits) - 1 };

	// invalid id for any id type -> means basically all bits are set to 1
	constexpr id_type id_mask{ id_type{-1} };


	using generation_type = std::conditional_t<generation_bits <= 16, std::conditional_t<generation_bits <= 8, u8, u16>, u32>;
	
	
	// some static asserts to make sure we don't f it up
	static_assert(sizeof(generation_type) * 8 >= generation_bits);
	static_assert((sizeof(id_type) - sizeof(generation_type)) > 0);


	// id is valid when it's not -1 (id_type is unsigned value type)
	inline bool is_valid(id_type id) {
		return id != id_mask;
	}

	inline id_type index(id_type id) {
		return id & index_mask;
	}

	inline id_type generation(id_type id) {
		return (id >> index_bits) & generation_mask;
	}

	inline id_type new_generation(id_type id) {
		const id_type generation{ id::generation(id) + 1 };

		// check if we don't exceed the max amount this unsigned type can hold -> e.g.
		// u8 -> 255
		// u16 -> 65535
		// u32 -> 4294967295
		// assert(generation < std::numeric_limits<generation_type>::max());
		
		// I am going to do this since I don't plan on ever having more entities than this setup can provide. (8 bits for generation, rest 24 bits for index)
		assert(generation < 255);
		return index(id) | (generation << index_bits);
	}



#if _DEBUG
	namespace internal
	{
		struct id_base
		{
			constexpr explicit id_base(id_type id) : _id{ id } {}

			// for doing e.g. u32 id = ...
			constexpr operator id_type() const { return _id; }

		private:
			id_type _id;
		};
	}


// one constructor takes value, other initializes with invalid index.
#define DEFINE_TYPED_ID(name)												\
	struct name final : id::internal::id_base								\
	{																		\
		constexpr explicit name(id::id_type id)								\
			: id_base{id} {}												\
																			\
		constexpr name() : id_base{id::id_mask} {}							\
	};
#else
#define DEFINE_TYPED_ID(name) using name = id::id_type;
#endif
}