struct GlobalShaderData
{
    float4x4 View;
    float4x4 Projection;
    float4x4 InvProjection;
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
             
    float3   CameraPosition;
    float    ViewWidth;
             
    float3   CameraDirection;
    float    ViewHeight;
             
    float    DeltaTime;
};

struct PerObjectData
{
    float4x4 World;
    float4x4 InvWorld;
    float4x4 WorldViewProjection;
};

struct VertexOut
{
    float4  HomogenousPosition   : SV_Position;
    float   WorldPosition        : POSITION;
    float   WorldNormal          : NORMAL;
    float   WorldTangent         : TANGENT;
    float2  UV                   : TEXTURE;
};

ConstantBuffer<GlobalShaderData>    PerFrameBuffer  : register(b0, space0);
ConstantBuffer<PerObjectData>       PerObjectBuffer : register(b1, space0);
StructuredBuffer<float3>            VertexPositions : register(t0, space0);

struct PixelOut
{
    float4 Color : SV_Target0;
};

VertexOut TestShaderVS(in uint VertexIdx : SV_VertexID)
{
    VertexOut vsOut;
    vsOut.HomogenousPosition = 0.f;
    vsOut.WorldPosition      = 0.f;
    vsOut.WorldNormal        = 0.f;
    vsOut.WorldTangent       = 0.f;
    vsOut.UV                 = 0.f;
    
    return vsOut;
}

[earlydepthstencil]
PixelOut TestShaderPS(in VertexOut psIn)
{
    PixelOut psOut;
    psOut.Color = 0.f;
    
    return psOut;
}