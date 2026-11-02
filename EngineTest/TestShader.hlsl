#include "Common.hlsli"

struct VertexOut
{
    float4  HomogenousPosition   : SV_Position;
    float3  WorldPosition        : POSITION;
    float3  WorldNormal          : NORMAL;
    float3  WorldTangent         : TANGENT;
    float2  UV                   : TEXTURE;
};

//struct ElementStaticNormalTexture
//{
//    uint        ColorTSign;
//    uint16_t2   Normal;
//    uint16_t2   Tangent;
//    float2      UV;
//};

struct PixelOut
{
    float4 Color : SV_Target0;
};

#define ELementsTypePositionOnly                0x00
#define ELementsTypeStaticNormal                0x01
#define ELementsTypeStaticNormalTexture         0x03 // normal and texture bits set to 1
#define ELementsTypeStaticColor                 0x04
#define ELementsTypeSkeletal                    0x08
#define ELementsTypeSkeletalColor               ELementsTypeSkeletal | ELementsTypeStaticColor
#define ELementsTypeSkeletalNormal              ELementsTypeSkeletal | ELementsTypeStaticNormal
#define ELementsTypeSkeletalNormalColor         ELementsTypeSkeletalNormal | ELementsTypeStaticColor
#define ELementsTypeSkeletalNormalTexture       ELementsTypeSkeletal | ELementsTypeStaticNormalTexture
#define ELementsTypeSkeletalNormalTextureColor  ELementsTypeSkeletalNormalTexture | ELementsTypeStaticColor

// #ifndef ELEMENTS_TYPE
// #define ELEMENTS_TYPE ELementsTypeStaticNormalTexture
// #endif

// depends on compile-time definition
struct VertexElement
{
#if ELEMENTS_TYPE == ELementsTypeStaticNormal
    uint        ColorTSign;
    uint16_t2   Normal;
#elif ELEMENTS_TYPE == ELementsTypeStaticNormalTexture
    uint        ColorTSign;
    uint16_t2   Normal;
    uint16_t2   Tangent;
    float2      UV;
#elif ELEMENTS_TYPE == ELementsTypeStaticColor
#elif ELEMENTS_TYPE == ELementsTypeSkeletal
#elif ELEMENTS_TYPE == ELementsTypeSkeletalColor
#elif ELEMENTS_TYPE == ELementsTypeSkeletalNormal
#elif ELEMENTS_TYPE == ELementsTypeSkeletalNormalColor
#elif ELEMENTS_TYPE == ELementsTypeSkeletalNormalTexture
#elif ELEMENTS_TYPE == ELementsTypeSkeletalNormalTextureColor
#endif
};

ConstantBuffer<GlobalShaderData>                GlobalData          : register(b0, space0);
ConstantBuffer<PerObjectData>                   PerObjectBuffer     : register(b1, space0);
StructuredBuffer<float3>                        VertexPositions     : register(t0, space0);
StructuredBuffer<VertexElement>                 Elements            : register(t1, space0);

StructuredBuffer<DirectionalLightParameters>    DirectionalLights   : register(t3, space0);

const static float InvIntervals = 2.f / ((1 << 16) - 1);

VertexOut TestShaderVS(in uint VertexIdx : SV_VertexID)
{
    // NOTE: when transforming position w = 1, where as for vectors w = 0. 
    //       This is done to ignore translations for vectors, since they only have direction.
    VertexOut vsOut;
    
    float4 position = float4(VertexPositions[VertexIdx], 1.f); // we get VertexIdx from render() function
    float4 worldPosition = mul(PerObjectBuffer.World, position);
    
    // (normal and texture bits set to 1)
#if ELEMENTS_TYPE == ELementsTypeStaticNormal
    VertexElement element = Elements[VertexIdx];
    float2 nXY = element.Normal * InvIntervals - 1.f;
    uint signs = (element.ColorTSign >> 24) & 0xff;
    float nSign = float(signs & 0x02) - 1;

    // normals are packed using math::pack_unit_float<16>(f32 f) function, so similar to the math::unpack_to_unit_float<16>(u32 i)
    float3 normal = float3(nXY.x, nXY.y, sqrt(saturate(1.f - dot(nXY, nXY))) * nSign);
    
    vsOut.HomogenousPosition = mul(PerObjectBuffer.WorldViewProjection, position); // alternatively you can multiply worldPosition by ViewProjection 
    vsOut.WorldPosition = worldPosition.xyz;
    vsOut.WorldNormal = mul(float4(normal, 0.f), PerObjectBuffer.InvWorld).xyz;
    vsOut.WorldTangent = 0.f;
    vsOut.UV = 0.f;
#elif ELEMENTS_TYPE == ELementsTypeStaticNormalTexture
    VertexElement element = Elements[VertexIdx];
    float2 nXY = element.Normal * InvIntervals - 1.f;
    uint signs = (element.ColorTSign >> 24) & 0xff;
    float nSign = float(signs & 0x02) - 1;

    // normals are packed using math::pack_unit_float<16>(f32 f) function, so similar to the math::unpack_to_unit_float<16>(u32 i)
    float3 normal = float3(nXY.x, nXY.y, sqrt(saturate(1.f - dot(nXY, nXY))) * nSign);
    
    vsOut.HomogenousPosition = mul(PerObjectBuffer.WorldViewProjection, position); // alternatively you can multiply worldPosition by ViewProjection 
    vsOut.WorldPosition = worldPosition.xyz;
    vsOut.WorldNormal = mul(float4(normal, 0.f), PerObjectBuffer.InvWorld).xyz;
    vsOut.WorldTangent = 0.f;
    vsOut.UV = 0.f;
#else
#undef ELEMENTS_TYPE
    vsOut.HomogenousPosition = mul(PerObjectBuffer.WorldViewProjection, position); // alternatively you can multiply worldPosition by ViewProjection 
    vsOut.WorldPosition = worldPosition.xyz;
    vsOut.WorldNormal = 0.f;
    vsOut.WorldTangent = 0.f;
    vsOut.UV = 0.f;
#endif
    
    return vsOut;
}

[earlydepthstencil]
PixelOut TestShaderPS(in VertexOut psIn)
{
    PixelOut psOut;
    
    float3 normal = normalize(psIn.WorldNormal); // interpolated value, it's possible it has non-unit length => normalize
    float3 viewDir = normalize(GlobalData.CameraPosition - psIn.WorldPosition);
    
    float3 color = 0;
    
    for (uint i = 0; i < GlobalData.NumDirectionalLights; ++i)
    {
        // calculate contribution of each directional light to this pixel
        DirectionalLightParameters light = DirectionalLights[i];
        
        float3 lightDirection = light.Direction;
        if (abs(lightDirection.z - 1.f) < 0.001f)
        {
            lightDirection = GlobalData.CameraDirection; // light object from whenever camera is.
        }
        
        float diffuse = max(dot(normal, -lightDirection), 0.f); // cosine of the angle between surface normal and the vector towards the light
        float3 reflection = reflect(lightDirection, normal);
        float specular = pow(max(dot(viewDir, reflection), 0.f), 16) * 0.5f;

        float3 lightColor = light.Color * light.Intensity;
        color += (diffuse + specular) * lightColor;
    }
    
    float3 ambient = 10 / 255.f;
    psOut.Color = saturate(float4(color + ambient, 1.f)); 
    
    // psOut.Color = float4(psIn.WorldNormal, 1.f);
    return psOut;
}