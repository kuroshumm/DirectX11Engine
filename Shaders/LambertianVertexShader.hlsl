#include "Lambertian.hlsli"

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    float4 pos = float4(input.Pos, 1.0f);
    pos = mul(pos, World);
    pos = mul(pos, View);
    pos = mul(pos, Projection);
    output.Pos = pos;

    output.Norm = normalize(mul(float4(input.Normal, 0.0f), World).xyz);

    // 接線ベクトルのワールド空間変換 (平行移動成分を無効化)
    output.Tangent = normalize(mul(float4(input.Tangent, 0.0f), World).xyz);

    output.Color = input.Color;
    output.TexCoord = input.TexCoord;

    return output;
}