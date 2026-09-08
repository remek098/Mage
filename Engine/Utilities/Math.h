#pragma once

#include "CommonHeaders.h"
#include "MathTypes.h"

namespace mage::math {
    template<typename T>
    constexpr T clamp(T value, T min, T max) {
        return (value < min) ? min : (value > max) ? max : value;
    }

    /// <summary>
    /// Pack a floating point value that is between [0.f, 1.f] into integer (amount of bits defined by bits type param
    /// </summary>
    /// <typeparam name="bits">Amount of bits of the returned u32 into which floating point value should be packed.</typeparam>
    /// <param name="f"> -> value between [0.f, 1.f] which will be packed into bits amount of bits inside returned u32.</param>
    /// <returns></returns>
    template<u32 bits> 
    constexpr u32 pack_unit_float(f32 f) {
        static_assert(bits <= sizeof(u32) * 8);
        assert(f >= 0.f && f <= 1.f);

        // 1ui32 -> 1u is treated by compiler as uint32_t
        constexpr f32 intervals = (f32)((1ui32 << bits) - 1);
        return (u32)(intervals * f + 0.5f); // account for rounding of floating point values into integer
    }

    template<u32 bits>
    constexpr f32 unpack_to_unit_float(u32 i) {
        static_assert(bits <= sizeof(u32) * 8);
        assert(i < (1ui32 << bits));
        constexpr f32 intervals = (f32)((1ui32 << bits) - 1);
        return (f32)i / intervals;
    }

    template<u32 bits>
    constexpr u32 pack_float(f32 f, f32 min, f32 max) {
        assert(min < max);
        assert(f <= max && f >= min);
        const f32 distance = (f - min) / (max - min); // scaling to range [0.f, 1.f]
        return pack_unit_float<bits>(distance);
    }

    template<u32 bits>
    constexpr u32 unpack_to_float(u32 i, f32 min, f32 max) {
        assert(min < max);
        return unpack_to_unit_float<bits>(i) * (max - min) + min; // literally reverse of pack_float
    }

    // align by rounding up. Will result in a multiple of "alignment"
    template<u64 alignment>
    constexpr u64 align_size_up(u64 size) {
        /*
        nice and fast way of checking if a single bit is set: value && !(value & (value - 1)):
        value:      0010 0000
        value-1:    0001 1111
        This gives  !(0010 0000 & 0001 1111) => 1111 1111
        i.e. if any of bits after most significant set bit is set, therefore value & (value -1) would be non-zero
        */
        static_assert(alignment, "Alignment must be non-zero."); // non-zero alignment is required.
        constexpr u64 mask{ alignment - 1 };
        static_assert(!(alignment & mask), "Alignment should be a power of 2."); // the !(value & (value -1)) part

        
        // add a mask and clean up mask bits.
        return((size + mask) & ~mask);
    }

    // align by rounding down. Will result in a multiple of "alignment"
    template<u64 alignment>
    constexpr u64 align_size_down(u64 size) {
        static_assert(alignment, "Alignment must be non-zero.");
        constexpr u64 mask{ alignment - 1 };
        static_assert(!(alignment & mask), "Alignment should be a power of 2.");
        // clean up mask bits -> i.e. align to most significant bit's power of 2.
        return(size & ~mask);
    }
}