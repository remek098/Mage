#include "Fractals.hlsli"

struct ShaderConstants
{
    float   Width;
    float   Height;
    uint    Frame;
};

// b1 register is where our constant root 32bit parameters are mapped to 
// in D3D12PostProcess.cpp: parameters[idx::root_constants].as_constant(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
ConstantBuffer<ShaderConstants> ShaderParams : register(b1);

#define SAMPLES 4

float4 FillColorPS(in noperspective float4 Position : SV_Position,
                   in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    // simple implementation of multisampling by calculating each pixel multiple times, but each time
    // move them around slightly and then taking the avarage sum of those pixel's colors.
    const float offset = 0.2f;
    const float2 offsets[4] =
    {
        float2(-offset, offset),
        float2(offset, offset),
        float2(offset, -offset),
        float2(-offset, -offset)
    };
    
    const float2 invDim = float2(1.f / ShaderParams.Width, 1.f/ShaderParams.Height);
    
    float3 color = 0.f;
    for (int i = 0; i < SAMPLES; i++)
    {
        const float2 uv = (Position.xy + offsets[i]) * invDim;
        // color += DrawMandelbrot(uv);
        color += DrawJuliaSet(uv, ShaderParams.Frame, 0.0008f);
    }
    
    return float4(float3(color.z, color.x, 1.f) * color.x / SAMPLES, 1.f);
    //return float4(color / SAMPLES, 1.f);
}