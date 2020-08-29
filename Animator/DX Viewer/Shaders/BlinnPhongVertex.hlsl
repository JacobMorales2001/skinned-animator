#include "utility.hlsl"

VertexShaderOutput main(AppData IN) //Simple vertex shader
{
    VertexShaderOutput OUT;
    
    float4 skinned_pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 skinned_norm = { 0.0f, 0.0f, 0.0f, 0.0f };
    [unroll]
    for (int j = 0; j < 4; ++j)
    {
        skinned_pos += mul(float4(IN.Position.xyz, 1.0f), JointTransforms[IN.Joints[j]]) * IN.Weights[j];
        skinned_norm += mul(float4(IN.Normal.xyz, 0.0f), JointTransforms[IN.Joints[j]]) * IN.Weights[j];
    }
    
    OUT.Position = skinned_pos + IN.InstancePos;
 
    OUT.Position = mul(OUT.Position, WorldViewProjectionMatrix);
    OUT.PositionWS = mul(skinned_pos, WorldMatrix);
    OUT.NormalWS = mul(skinned_norm, InverseTransposeWorldMatrix).xyz;
    OUT.TexCoord = IN.TexCoord;
 
    return OUT;
}

//VS_Output main(VS_Input input)
//{
//    VS_Output output;
    
//    output.positionWS = mul(float4(input.position.xyz, 1.0f), World);
//    //output.position = mul(output.positionWS, View);
//    //output.position = mul(output.position, Projection);
    
//    //output.eye_pos.x = -dot(View[3].xyz, View[0].xyz);
//    //output.eye_pos.y = -dot(View[3].xyz, View[1].xyz);
//    //output.eye_pos.z = -dot(View[3].xyz, View[2].xyz);
//    //output.eye_pos.w = 1.0f;
    
//    //output.normal = mul(float4(input.normal.xyz, 0.0f), World);
    
//    output.tex = input.uv.xy;
    
//    output.normalWS = mul(float4(input.normal.xyz, 0.0f), World);
    
//    //output.view = mul(input.position, World).xyz - output.positionWS.xyz;
    
//    //output.color = input.color;
    
//    return output;
//}

