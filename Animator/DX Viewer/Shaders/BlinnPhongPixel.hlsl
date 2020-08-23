#include "utility.hlsl"

//static const float4 ambient_light = { 0.5f, 0.5f, 0.5f, 0.0f };

float4 main(PixelShaderInput IN) : SV_TARGET
{
    LightingResult lit = ComputeLighting(IN.PositionWS, normalize(IN.NormalWS));
     
    float4 diffuse = Material.Diffuse * lit.Diffuse * txDiffuse.Sample(g_sampler, IN.TexCoord) * Material.DiffuseFactor;
    //float4 diffuse = { 0.0f, 0.0f, 0.0f, 0.0f };
    //float4 ambient = Material.Ambient * GlobalAmbient;// * Material.AmbientFactor;
    float4 ambient = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 emissive = Material.Emissive * txEmissive.Sample(g_sampler, IN.TexCoord) * Material.EmissiveFactor;
    //float4 emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 specular = Material.Specular * lit.Specular * txSpecular.Sample(g_sampler, IN.TexCoord)* Material.SpecularFactor;
 
    //float4 texColorDiffuse = { 1, 1, 1, 1 };
    //float4 texColorEmissive = { 1, 1, 1, 1 };
    //float4 texColorSpecular = { 1, 1, 1, 1 };
     
    //if (Material.UseTexture)
    //{
    //    texColorDiffuse = txDiffuse.Sample(g_sampler, IN.TexCoord) * Material.DiffuseFactor;
    //    texColorEmissive = txEmissive.Sample(g_sampler, IN.TexCoord) * Material.EmissiveFactor;
    //    texColorSpecular = txSpecular.Sample(g_sampler, IN.TexCoord) * Material.SpecularFactor;
    //}
 
    float4 finalColor = saturate(emissive + ambient + diffuse + specular); //* texColorDiffuse * texColorEmissive * texColorSpecular;
 
    return finalColor;
}

//PS_Output main(PS_Input input)
//{
//    float4 finalColor = 0;
//    float4 finalDiffuse = 0;
//    float4 finalSpecular = 0;
    
//    //do directional lighting (diffuse)
//    finalColor += saturate(dot((float3) vLightDir[0], input.normal) * vLightColor[0]);
//    //ambient
//    //point light from vertex shader.
//    //Do point light calculation
//    float3 tmp = vLightPosition[1].xyz - input.position.xyz;
    
//    float lightRatio = saturate(dot(input.normal, normalize(tmp)));
//    float attenuation = pow(1.0f - saturate(length(tmp) / vPointLightRadius), 2);
//    lightRatio *= attenuation;
    
//    finalColor *= input.color + lerp(0, vLightColor[1], saturate(lightRatio));
//    finalColor *= input.color;
    
//    finalColor += ambient_light;
//    //Spot light calc
//    float4 SpotLightDiffuse;
//    float4 SpotLightSpecular;
//    float3 V = normalize(View[3] - input.positionWS).xyz;
//    float3 P = input.positionWS;
//    float3 N = normalize(input.normalWS);
//    float3 L = (vLightPosition[2].xyz - P).xyz;
//    float distance = length(L);
//    L = L / distance;
    
//    // Linear attenuation is 0.08f
//    attenuation = 1.0f / (1.0f + (0.08f * distance));
//    float spotIntensity;
//    {
//        float minCos = cos(1.570796f); // PI/2
//        float maxCos = (minCos + 1.0f) / 2.0f;
//        float cosAngle = dot(vLightDir[2].xyz, -L);
//        spotIntensity = smoothstep(minCos, maxCos, cosAngle);
//    }
    
    
//    float3 R = normalize(reflect(-L, N));
//    float RdotV = max(0, dot(R, V));
//    float NdotL = max(0, dot(N, L));
    
//    SpotLightDiffuse = vLightColor[2] * NdotL * attenuation * spotIntensity;
//    SpotLightSpecular = vLightColor[2] * pow(RdotV, 76.8f) * attenuation * spotIntensity;
    
//    finalDiffuse += SpotLightDiffuse;
//    finalSpecular += SpotLightSpecular;
    
//    float4 DirectionalLightDiffuse;
//    float4 DirectionalLightSpecular;
//    L = -vLightPosition[0].xyz;
//    distance = length(L);
//    L = L / distance;
    
//    R = normalize(reflect(-L, N));
//    RdotV = max(0, dot(R, V));
//    NdotL = max(0, dot(N, L));
    
//    DirectionalLightDiffuse = vLightColor[0] * NdotL;
//    DirectionalLightSpecular = vLightColor[0] * pow(RdotV, 76.8f);
    
//    finalDiffuse += DirectionalLightDiffuse;
//    finalSpecular += DirectionalLightSpecular;
    
//    float4 PointLightDiffuse;
//    float4 PointLightSpecular;
//    L = (vLightPosition[0].xyz - P).xyz;
//    distance = length(L);
//    L = L / distance;
    
//    R = normalize(reflect(-L, N));
//    RdotV = max(0, dot(R, V));
//    NdotL = max(0, dot(N, L));
    
//    attenuation = 1.0f - distance / vPointLightRadius;
//    PointLightDiffuse = vLightColor[1] * NdotL * attenuation;
//    PointLightSpecular = vLightColor[1] * pow(RdotV, 0.2f) * attenuation;
    
//    finalDiffuse += PointLightDiffuse;
//    finalSpecular += PointLightSpecular;
    
//    float4 DiffuseMod = float4(0.4f, 0.4f, 0.4f, 1.0f);
//    float4 SpecularMod = float4(0.774597f, 0.774597f, 0.774597f, 1.0f);
    
//    finalColor = (saturate(finalDiffuse) * DiffuseMod) + (saturate(finalSpecular) * SpecularMod);
//    finalColor.a = 1;
//    return saturate(finalColor);
//}

//PS_Output main(PS_Input input)
//{
//    PS_Output output;
    
//    // Normalize all vectors in pixel shader to get phong shading
//	// Normalizing in vertex shader would provide gouraud shading
//    float3 light = normalize(-vLightDir[1].xyz);
//    float3 view = normalize(input.view);
//    float3 normal = normalize(input.normal);
           
//	// Calculate the half vector
//    float3 halfway = normalize(light + view);
 
//	// Calculate the emissive lighting
//    float3 emissive = mEmissiveColor.xyz * txEmissive.Sample(g_sampler, input.tex).xyz;
//	// Calculate the ambient reflection
//    float3 ambient = ambient_light.xyz;
//	// Calculate the diffuse reflection
//    float3 diffuse = saturate(dot(normal, light)) * mDiffuseColor.rgb;
 
//	// Calculate the specular reflection
//    float3 specular = pow(saturate(dot(normal, halfway)), mSpecularColor.w) * mSpecularColor.xyz * txSpecular.Sample(g_sampler, input.tex).xyz;
    
//	// Fetch the texture coordinates
//    float2 texCoord = input.tex;
 
//	// Sample the texture
//    float4 texColor = txDiffuse.Sample(g_sampler, texCoord);
 
//	// Combine all the color components
//    float3 color = (saturate(ambient + diffuse) * texColor.xyz + specular) * vLightColor[1].xyz + emissive;
//	// Calculate the transparency
//    float alpha = mDiffuseColor.a * texColor.a;
//	// Return the pixel's color
//    output.color = float4(color, alpha);
//    return output;
//}



//PS_Output main(PS_Input input)
//{
//    PS_Output output;
//    float3 light_dir = vLightPosition[1].xyz - input.positionWS.xyz;
//    float sq_distance = dot(light_dir, light_dir);
    
//    light_dir = light_dir / sqrt(sq_distance);
//    float3 eye_dir = input.eye_pos.xyz - input.positionWS.xyz;
//    float sq_distance_eye = dot(eye_dir, eye_dir);
//    float distance_eye = sqrt(sq_distance_eye);
//    eye_dir = eye_dir / distance_eye;
//    float3 norm = normalize(input.normal.xyz);
//    float nl = dot(norm, light_dir);
//    float diffuse_intensity = saturate(nl);
//    float3 half_vector = normalize(light_dir + eye_dir);
//    float nh = dot(norm, half_vector);
//    float specular_intensity = pow(saturate(nh), 1 + mSpecularColor.w);
//    float4 light_intensity = float4(vLightColor[1].xyz, 0.0f) * vLightDir[1].x / sq_distance;
//    float4 mat_diffuse = txDiffuse.Sample(g_sampler, input.tex); // * float4(mDiffuseColor.xyz, 0.0f) * mDiffuseColor.w;
//    float4 mat_specular = txSpecular.Sample(g_sampler, input.tex); // * float4(mSpecularColor.xyz, 0.0f) * mSpecularColor.w;
//    float4 mat_emissive = txEmissive.Sample(g_sampler, input.tex); //* float4(mEmissiveColor.xyz, 0.0f) * mEmissiveColor.w;
//    float4 emissive = mat_emissive;
//    float4 ambient = mat_diffuse * ambient_light;
//    float4 specular = mat_specular * specular_intensity * light_intensity;
//    float4 diffuse = mat_diffuse * diffuse_intensity * light_intensity;
//    // hacky conservation of energy
//    diffuse.xyz -= specular.xyz;
//    diffuse.xyz = saturate(diffuse.xyz);
//    float4 color = ambient + specular + diffuse + emissive;
        
//    output.color = color;
//    return output;
//}