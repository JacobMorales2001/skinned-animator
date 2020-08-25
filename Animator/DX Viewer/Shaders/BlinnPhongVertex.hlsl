#include "utility.hlsl"

VertexShaderOutput main(AppData IN) //Simple vertex shader
{
    VertexShaderOutput OUT;
    
    OUT.Position = IN.Position + IN.InstancePos;
 
    OUT.Position = mul(OUT.Position, WorldViewProjectionMatrix);
    OUT.PositionWS = mul(IN.Position, WorldMatrix);
    OUT.NormalWS = mul(IN.Normal, InverseTransposeWorldMatrix).xyz;
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

