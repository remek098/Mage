
struct ShaderConstants
{
    uint GPassMainBufferIndex;
};

// b1 register is where our constant root 32bit parameters are mapped to 
// in D3D12PostProcess.cpp: parameters[idx::root_constants].as_constant(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
ConstantBuffer<ShaderConstants> ShaderParams : register(b1);
// the 2nd root param was descriptor table which is unbounded range in our resource descriptor heap
Texture2D                       Textures[]   : register(t0, space0);

float4 PostProcessPS(in noperspective float4 Position : SV_Position,
                     in noperspective float2 UV : TEXCOORD) : SV_Target0
{
    // samples each pixel in main buffer and outputs it into render target, which is swapchain's backbuffer.
    Texture2D gpassMain = Textures[ShaderParams.GPassMainBufferIndex];
    float4 color = float4(gpassMain[Position.xy].xyz, 1.f);
    return color;
}