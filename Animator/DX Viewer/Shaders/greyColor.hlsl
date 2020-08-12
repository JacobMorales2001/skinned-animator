#include "utility.hlsl"

//Texture2D txDiffuse : register(t0);
//SamplerState samLinear : register(s0);

//struct VS_INPUT
//{
//    float4 position : POSITION;
//    float3 normal : NORMAL;
//    float4 color : COLOR;
//};


float4 main(PS_Input input) : SV_TARGET
{
    float4 c = txDiffuse.Sample(g_sampler, input.tex);
    return float4(0.0f, 0.5f, 1.0f, 1.0f);
}