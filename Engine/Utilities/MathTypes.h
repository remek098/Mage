#pragma once

#include "CommonHeaders.h"
// #include "PrimitiveTypes.h"

namespace mage::math {
    constexpr f32 pi = 3.1415926535897932384626433832795f;
    constexpr f32 epsilon = 1e-5f;

#if defined(_WIN64)
    using vec2  = DirectX::XMFLOAT2;
    using vec2a = DirectX::XMFLOAT2A;
    using vec3  = DirectX::XMFLOAT3;
    using vec3a = DirectX::XMFLOAT3A;
    using vec4  = DirectX::XMFLOAT4;
    using vec4a = DirectX::XMFLOAT4A;

    using u32vec2 = DirectX::XMUINT2;
    using u32vec3 = DirectX::XMUINT3;
    using u32vec4 = DirectX::XMUINT4;

    using i32vec2 = DirectX::XMINT2;
    using i32vec3 = DirectX::XMINT3;
    using i32vec4 = DirectX::XMINT4;

    
    using mat3x3    = DirectX::XMFLOAT3X3; // NOTE: DirectXMath doesn't have aligned 3x3 matrices
    using mat4x4    = DirectX::XMFLOAT4X4;
    using mat4x4a   = DirectX::XMFLOAT4X4A;

#endif
}