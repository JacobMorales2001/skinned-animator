
struct PS_Input
{
    float4 position : SV_Position;
    float4 color : PIXELCOLOR;
};

float4 main(PS_Input input) : SV_TARGET
{
	return input.color;
}