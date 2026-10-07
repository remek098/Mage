#pragma once

#include "Graphics/Direct3D12/D3D12CommonHeaders.h"

namespace mage::gfx::d3d12::hlsl {

// mapping hlsl types
using float4x4 = math::mat4x4a;
using float3x3 = math::mat3x3;

using float4 = math::vec4;
using float3 = math::vec3;
using float2 = math::vec2;

using uint4 = math::u32vec4;
using uint3 = math::u32vec3;
using uint2 = math::u32vec2;
using uint = u32;


#include "CommonTypes.hlsli"
}