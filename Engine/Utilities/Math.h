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
}