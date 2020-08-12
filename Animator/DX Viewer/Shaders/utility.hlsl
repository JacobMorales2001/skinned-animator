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

Texture2D txDiffuse : register(t0);
Texture2D txEmissive : register(t1);
Texture2D txSpecular : register(t2);
SamplerState g_sampler : register(s0);

struct PS_Input
{
    float4 position : SV_POSITION;
    float4 positionWS : TEXCOORD1;
    float3 normalWS   : TEXCOORD2;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 tex : TEX;
};

struct VS_Input
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 color : COLOR;
    float4 uv : TEX;
};

struct GS_Output
{
    float4 posH : SV_POSITION;
};


