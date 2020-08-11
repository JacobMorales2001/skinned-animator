#include "Utility.hlsl"

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
    float4 finalColor = 0;
    float4 finalDiffuse = 0;
    float4 finalSpecular = 0;
    
    //do directional lighting (diffuse)
    finalColor += saturate(dot((float3) vLightDir[0], input.normal) * vLightColor[0]);
    //ambient
    //point light from vertex shader.
    finalColor *= input.color;
    
    finalColor += 0.2f;
    //Spot light calc
    float4 SpotLightDiffuse;
    float4 SpotLightSpecular;
    float3 V = normalize(View[3] - input.positionWS).xyz;
    float3 P = input.positionWS;
    float3 N = normalize(input.normalWS);
    float3 L = (vLightPosition[2].xyz - P).xyz;
    float distance = length(L);
    L = L / distance;
    
    // Linear attenuation is 0.08f
    float attenuation = 1.0f / (1.0f + (0.08f * distance));
    float spotIntensity;
    {
        float minCos = cos(1.570796f); // PI/2
        float maxCos = (minCos + 1.0f) / 2.0f;
        float cosAngle = dot(vLightDir[2].xyz, -L);
        spotIntensity = smoothstep(minCos, maxCos, cosAngle);
    }
    
    
    float3 R = normalize(reflect(-L, N));
    float RdotV = max(0, dot(R, V));
    float NdotL = max(0, dot(N, L));
    
    SpotLightDiffuse = vLightColor[2] * NdotL * attenuation * spotIntensity;
    SpotLightSpecular = vLightColor[2] * pow(RdotV, 76.8f) * attenuation * spotIntensity;
    
    finalDiffuse += SpotLightDiffuse;
    finalSpecular += SpotLightSpecular;
    
    float4 DirectionalLightDiffuse;
    float4 DirectionalLightSpecular;
    L = -vLightPosition[0].xyz;
    distance = length(L);
    L = L / distance;
    
    R = normalize(reflect(-L, N));
    RdotV = max(0, dot(R, V));
    NdotL = max(0, dot(N, L));
    
    DirectionalLightDiffuse = vLightColor[0] * NdotL;
    DirectionalLightSpecular = vLightColor[0] * pow(RdotV, 76.8f);
    
    finalDiffuse += DirectionalLightDiffuse;
    finalSpecular += DirectionalLightSpecular;
    
    float4 PointLightDiffuse;
    float4 PointLightSpecular;
    L = (vLightPosition[0].xyz - P).xyz;
    distance = length(L);
    L = L / distance;
    
    R = normalize(reflect(-L, N));
    RdotV = max(0, dot(R, V));
    NdotL = max(0, dot(N, L));
    
    attenuation = 1.0f - distance / vPointLightRadius;
    PointLightDiffuse = vLightColor[1] * NdotL * attenuation;
    PointLightSpecular = vLightColor[1] * pow(RdotV, 0.2f) * attenuation;
    
    finalDiffuse += PointLightDiffuse;
    finalSpecular += PointLightSpecular;
    
    float4 DiffuseMod = float4(0.4f, 0.4f, 0.4f, 1.0f);
    float4 SpecularMod = float4(0.774597f, 0.774597f, 0.774597f, 1.0f);
    
    finalColor = (saturate(finalDiffuse) * DiffuseMod) + (saturate(finalSpecular) * SpecularMod);
    finalColor.a = 1;
    return saturate(finalColor);
}