#include "utility.hlsl"

//#include "phong_material.hlsli"


static const float4 ambient_light = { 0.5f, 0.5f, 0.5f, 0.0f };
PS_OUTPUT main(PS_Input input)
{
    PS_OUTPUT output;
    float3 light_dir = vLightPosition[1].xyz - input.positionWS.xyz;
    float sq_distance = dot(light_dir, light_dir);
    
    light_dir = light_dir / sqrt(sq_distance);
    float3 eye_dir = input.eye_pos.xyz - input.positionWS.xyz;
    float sq_distance_eye = dot(eye_dir, eye_dir);
    float distance_eye = sqrt(sq_distance_eye);
    eye_dir = eye_dir / distance_eye;
    float3 norm = normalize(input.normal.xyz);
    float nl = dot(norm, light_dir);
    float diffuse_intensity = saturate(nl);
    float3 half_vector = normalize(light_dir + eye_dir);
    float nh = dot(norm, half_vector);
    float specular_intensity = pow(saturate(nh), 1 + mSpecularColor.w);
    float4 light_intensity = float4(vLightColor[1].xyz, 0.0f) * vLightDir[1].x / sq_distance;
    float4 mat_diffuse = txDiffuse.Sample(g_sampler, input.tex); // *float4(surface_diffuse, 0.0f) * surface_diffuse_factor;
    float4 mat_specular = txSpecular.Sample(g_sampler, input.tex); // *float4(surface_specular, 0.0f) * surface_specular_factor;
    float4 mat_emissive = txEmissive.Sample(g_sampler, input.tex); // *float4(surface_emissive, 0.0f) * surface_emissive_factor;
    float4 emissive = mat_emissive;
    float4 ambient = mat_diffuse * ambient_light;
    float4 specular = mat_specular * specular_intensity * light_intensity;
    float4 diffuse = mat_diffuse * diffuse_intensity * light_intensity;
    // hacky conservation of energy
    diffuse.xyz -= specular.xyz;
    diffuse.xyz = saturate(diffuse.xyz);
    float4 color = ambient + specular + diffuse + emissive;
        
    output.color = color;
    return output;
}