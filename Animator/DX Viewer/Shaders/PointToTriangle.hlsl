#include "Utility.hlsl"
// the output stream must contain clip space verticies (if sent to the rasterizer)
//struct GSOutput
//{
//    float4 clr : COLOR;
//    float4 posH : SV_POSITION;
//};
// you may not exceed 1024 scalars. the max number of scalars output from this shader is 24
[maxvertexcount(3)]
// this operation builds each point into a blade of grass 
void main(point float4 input[1] : SV_POSITION, inout TriangleStream<GS_Output> output)
{

	float halfBW = 1 * 0.5;
	GS_Output simple[3];
	// positions
	simple[0].posH = float4(input[0].xyz,1);
	simple[1].posH = simple[0].posH;
	simple[2].posH = simple[0].posH;
	// shift to make triangle
	simple[0].posH.x -= halfBW;
	simple[1].posH.y += 1;
	simple[2].posH.x += halfBW;
	// move to projection space
	for(uint i = 0; i < 3; ++i)
	{
		simple[i].posH = mul(simple[i].posH, World);
		simple[i].posH = mul(simple[i].posH, View);
		simple[i].posH = mul(simple[i].posH, Projection);
		output.Append(simple[i]);
	}
}