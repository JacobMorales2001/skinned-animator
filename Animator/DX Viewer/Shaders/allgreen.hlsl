#include "Utility.hlsl"



float4 main(PS_Input input) : SV_TARGET
{
    // Force the color to green.
    return g_texture.Sample(g_sampler, input.tex);
}