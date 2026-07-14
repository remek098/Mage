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

float4 FillColorPS(in noperspective float4 Position : SV_Position,
                            in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    const float2 invDim = float2(1.f / ShaderParams.Width, 1.f/ShaderParams.Height);
    const float2 uv = Position.xy * invDim;
    float3 color = DrawMandelbrot(uv);
    return float4(color, 1.f);
}