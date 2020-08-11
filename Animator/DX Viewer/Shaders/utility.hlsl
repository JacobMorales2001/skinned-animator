cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 vLightDir[3];
    float4 vLightColor[3];
    float4 vLightPosition[3];
    float4 vOutputColor;
    float4 vPointLightRadius;
    matrix InverseTransposeWorld;
}

Texture2D g_texture : register(t0);
texture2D g_texture1 : register(t1);
SamplerState g_sampler : register(s0);

struct PS_Input
{
    float4 position : SV_POSITION;
    float4 positionWS : TEXCOORD1;
    float3 normalWS   : TEXCOORD2;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 tex : TEX;
    uint useTex : BOOLTEX;
};

struct VS_Input
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float4 uv : TEX;
    float3 instancePosition : INSTANCEPOS;
};

struct GS_Output
{
    float4 posH : SV_POSITION;
};

