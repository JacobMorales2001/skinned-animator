#include "utility.hlsl"

PS_Input main(VS_Input input)
{
    PS_Input output;
    output.positionWS = mul(float4(input.position.xyz, 1.0f), World);
    output.position = mul(output.positionWS, View);
    output.position = mul(output.position, Projection);
    output.eye_pos.x = -dot(View[3].xyz, View[0].xyz);
    output.eye_pos.y = -dot(View[3].xyz, View[1].xyz);
    output.eye_pos.z = -dot(View[3].xyz, View[2].xyz);
    output.eye_pos.w = 1.0f;
    output.normal = mul(float4(input.normal.xyz, 0.0f), World);
    output.tex = input.uv.xy;
    output.normalWS = float3(0.0f, 0.0f, 0.0f);
    output.color = input.color;
    return output;
}