// 第12回 PBR 演習用
// PBRVertexShader.hlsl

cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 LightDir;
    float4 LightColor;
    float4 Ambient;
    float4 CameraPos; // 視線ベクトル計算用
}

cbuffer PaletteBuffer : register(b1)
{
    matrix Bones[128];
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    uint4  BlendIndices : BLENDINDICES;
    float4 BlendWeight : BLENDWEIGHT;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT)0;

    float3 skinnedPos = float3(0.0f, 0.0f, 0.0f);
    float3 skinnedNormal = float3(0.0f, 0.0f, 0.0f);

    for(int boneIndex = 0; boneIndex < 4; ++boneIndex)
    {
        uint blendIndex = input.BlendIndices[boneIndex];
        float blendWeight = input.BlendWeight[boneIndex];

        skinnedPos += mul(float4(input.Pos, 1.0f), Bones[blendIndex]).xyz * blendWeight;
        skinnedNormal += mul(input.Normal, (float3x3)Bones[blendIndex]) * blendWeight;
    }

    
    
    // オブジェクト空間からワールド空間へ
    float4 worldPos = mul(float4(skinnedPos, 1.0f), World);
    output.WorldPos = worldPos.xyz;
    
    // 射影空間へ
    float4 viewPos = mul(worldPos, View);
    output.Pos = mul(viewPos, Projection);
    
    // 法線の変換 (ローカル->ワールド)
    output.Normal = normalize(mul(skinnedNormal, (float3x3)World));
    
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    
    return output;
}
