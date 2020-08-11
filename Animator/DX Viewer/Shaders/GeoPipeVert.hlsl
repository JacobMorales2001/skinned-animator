#include "Utility.hlsl"

//struct VS_INPUT
//{
//    float4 position : POSITION;
//    float3 normal : NORMAL;
//    float4 color : COLOR;
//};

PS_Input main(VS_Input input)
{
    PS_Input result;
    
    result.position = input.position;
    
    result.position.x += input.instancePosition.x;
    result.position.y += input.instancePosition.y;
    result.position.z += input.instancePosition.z;

    //result.position = mul(result.position, World);
    
    //Do point light calculation
    float3 tmp =
    {
        vLightPosition[1].x - input.position.x,
        vLightPosition[1].y - input.position.y,
        vLightPosition[1].z - input.position.z
    };
    
    float lightRatio = saturate(dot(input.normal, normalize(tmp)));
    float attenuation = pow(1.0f - saturate(length(tmp) / vPointLightRadius), 2);
    lightRatio *= attenuation;
    
    //result.color = input.color + lerp(0, vLightColor[1], saturate(lightRatio));
    result.color = input.color;
    result.position = mul(result.position, View);
    result.position = mul(result.position, Projection);
    
    result.normal = input.normal;
    
    result.tex = input.uv;
    
    result.positionWS = input.position, World;
    result.normalWS = input.normal, (float3x3) World;

    return result;
}
