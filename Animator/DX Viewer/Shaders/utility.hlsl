//cbuffer ConstantBuffer : register(b0)
//{
//    matrix World;
//    matrix View;
//    matrix Projection;
//    float4 vLightDir[3];
//    float4 vLightColor[3];
//    float4 vLightPosition[3];
//    float4 vOutputColor;
//    float4 vPointLightRadius;
//    matrix InverseTransposeWorld;
//    float4 mDiffuseColor;
//    float4 mEmissiveColor;
//    float4 mSpecularColor;
//}

//struct VS_Output
//{
//    float4 position : SV_POSITION;
//    float4 positionWS : TEXCOORD1;
//    float3 normalWS   : TEXCOORD2;
//    float3 normal : NORMAL;
//    float4 color : COLOR;
//    float2 tex : TEX;
//    float4 eye_pos : EYEPOS;
//    float3 view : VIEW;
//};

//struct VS_Output
//{
//    float4 positionWS : TEXCOORD1;
//    float3 normalWS : TEXCOORD2;
//    float2 tex : TEXCOORD0;
//};

//struct VS_Input
//{
//    float4 position : POSITION;
//    float4 normal : NORMAL;
//    float4 color : COLOR;
//    float4 uv : TEX;
//};

//struct PS_Output
//{
//    float4 color : SV_TARGET;
//};

//#define PS_Input VS_Output // The output of the Vertex shader is the input of the Pixel shader.

#define MAX_LIGHTS 8
 
// Light types.

#ifdef __cplusplus
   
enum ShaderLightType
{
    DIRECTIONAL_LIGHT = 0,
    POINT_LIGHT = 1,
    SPOT_LIGHT = 2,
    COUNT = 3
};

#endif


#ifndef __cplusplus

#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2


struct AppData
{
    float4 Position : POSITION;
    float4 Normal : NORMAL;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS : TEXCOORD2;
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_Position;
};

struct PixelShaderInput
{
    float4 PositionWS : TEXCOORD1;
    float3 NormalWS : TEXCOORD2;
    float2 TexCoord : TEXCOORD0;
};

Texture2D txDiffuse : register(t0);
Texture2D txEmissive : register(t1);
Texture2D txSpecular : register(t2);
SamplerState g_sampler : register(s0);


struct _Material
{
    float4 Emissive; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float4 Ambient; // 16 bytes
    //------------------------------------(16 byte boundary)
    float4 Diffuse; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float4 Specular; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float EmissiveFactor; // 4 bytes
    float AmbientFactor; // 4 bytes
    float DiffuseFactor; // 4 bytes
    float SpecularFactor; // 4 bytes
    //----------------------------------- (16 byte boundary)
    float SpecularPower; // 4 bytes
    bool UseTexture; // 4 bytes
    float2 Padding; // 8 bytes
    //----------------------------------- (16 byte boundary)
    
}; // Total:               // 80 bytes ( 5 * 16 )

//cbuffer MaterialProperties : register(b0)
//{
//    _Material Material;
//};



struct Light
{
    float4 Position; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float4 Direction; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float4 Color; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float SpotAngle; // 4 bytes
    float ConstantAttenuation; // 4 bytes
    float LinearAttenuation; // 4 bytes
    float QuadraticAttenuation; // 4 bytes
    //----------------------------------- (16 byte boundary)
    int LightType; // 4 bytes
    float LightPower; // 4 bytes
    bool Enabled; // 4 bytes
    int Padding; // 4 bytes
    //----------------------------------- (16 byte boundary)
}; // Total:                           // 80 bytes (5 * 16)
 
cbuffer PerObject : register(b0)
{
    matrix WorldMatrix; 
    matrix InverseTransposeWorldMatrix;
    matrix WorldViewProjectionMatrix;
    _Material Material;
    
    float4 EyePosition; // 16 bytes
    //----------------------------------- (16 byte boundary)
    float4 GlobalAmbient; // 16 bytes
    //----------------------------------- (16 byte boundary)
    Light Lights[MAX_LIGHTS]; // 80 * 8 = 640 bytes
};  // Total:                           // 672 bytes (42 * 16)

struct LightingResult
{
    float4 Diffuse;
    float4 Specular;
};

float4 DoDiffuse(Light light, float3 L, float3 N)
{
    float NdotL = max(0, dot(N, L));
    return light.Color * NdotL;
}

float4 DoSpecular(Light light, float3 V, float3 L, float3 N)
{
    // Phong lighting.
    //float3 R = normalize(reflect(-L, N));
    //float RdotV = max(0, dot(R, V));
 
    // Blinn-Phong lighting
    float3 H = normalize(L + V);
    float NdotH = max(0, dot(N, H));
 
    return light.Color * pow(NdotH, Material.SpecularPower);
}

float DoAttenuation(Light light, float d)
{
    return 1.0f / (light.ConstantAttenuation + light.LinearAttenuation * d + light.QuadraticAttenuation * d * d);
}
 
LightingResult DoPointLight(Light light, float3 V, float4 P, float3 N)
{
    LightingResult result;
 
    float3 L = (light.Position - P).xyz;
    float distance = length(L);
    L = L / distance;
 
    float attenuation = DoAttenuation(light, distance);
 
    result.Diffuse = DoDiffuse(light, L, N) * attenuation;
    result.Specular = DoSpecular(light, V, L, N) * attenuation;
 
    return result;
}

LightingResult DoDirectionalLight(Light light, float3 V, float4 P, float3 N)
{
    LightingResult result;
 
    float3 L = -light.Direction.xyz;
 
    result.Diffuse = DoDiffuse(light, L, N);
    result.Specular = DoSpecular(light, V, L, N);
 
    return result;
}

float DoSpotCone(Light light, float3 L)
{
    float minCos = cos(light.SpotAngle);
    float maxCos = (minCos + 1.0f) / 2.0f;
    float cosAngle = dot(light.Direction.xyz, -L);
    return smoothstep(minCos, maxCos, cosAngle);
}

LightingResult DoSpotLight(Light light, float3 V, float4 P, float3 N)
{
    LightingResult result;
 
    float3 L = (light.Position - P).xyz;
    float distance = length(L);
    L = L / distance;
 
    float attenuation = DoAttenuation(light, distance);
    float spotIntensity = DoSpotCone(light, L);
 
    result.Diffuse = DoDiffuse(light, L, N) * attenuation * spotIntensity;
    result.Specular = DoSpecular(light, V, L, N) * attenuation * spotIntensity;
 
    return result;
}

LightingResult ComputeLighting(float4 P, float3 N)
{
    float3 V = normalize(EyePosition - P).xyz;
 
    LightingResult totalResult = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
 
    [unroll]
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        LightingResult result = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 } };
 
        if (!Lights[i].Enabled)
            continue;
         
        switch (Lights[i].LightType)
        {
            case DIRECTIONAL_LIGHT:
            {
                    result = DoDirectionalLight(Lights[i], V, P, N);
                }
                break;
            case POINT_LIGHT:
            {
                    result = DoPointLight(Lights[i], V, P, N);
                }
                break;
            case SPOT_LIGHT:
            {
                    result = DoSpotLight(Lights[i], V, P, N);
                }
                break;
        }
        totalResult.Diffuse += result.Diffuse * Lights[i].LightPower;
        totalResult.Specular += result.Specular * Lights[i].LightPower;
    }
 
    totalResult.Diffuse = saturate(totalResult.Diffuse);
    totalResult.Specular = saturate(totalResult.Specular);
 
    return totalResult;
}



#endif //!cplusplus
