#include "Lambertian.hlsli"

float4 PSMain(PS_INPUT input) : SV_Target
{
    float3 N = normalize(input.Norm);
    float3 L = normalize(-LightDir.xyz);

    float diffuseFactor = saturate(dot(N, L));
    float4 diffuseLight = LightColor * diffuseFactor;

    // TODO: 法線マップ適用時に input.Tangent を利用してTBN行列を構築する
    
    float4 texColor = txDiffuse.Sample(samLinear, input.TexCoord);

    float4 finalColor = saturate(diffuseLight + Ambient) * texColor * input.Color;
    finalColor.a = texColor.a * input.Color.a;

    return finalColor;
}