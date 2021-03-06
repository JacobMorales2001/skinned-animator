#include "utility.hlsl"

struct VS_Output
{
    float4 position : SV_Position;
    float4 color : PIXELCOLOR;
};

VS_Output main( AppData input )
{
    VS_Output output;
    
    output.position = input.Position + input.InstancePos;
    
    output.position = mul(output.position, WorldViewProjectionMatrix);
    
    output.color = input.Color;
    
	return output;
}