// 第12回 PBR 演習用
// PBRPixelShader.hlsl

// --- b0 レジスタ: フレーム単位の定数バッファ (Graphics.cpp等で更新される) ---
cbuffer PerFrameBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
    float4 LightDir;
    float4 LightColor;
    float4 Ambient;
    float4 CameraPos; // 今回追加された位置
}

// --- b1 レジスタ: マテリアル単位の定数バッファ (Material.cppで更新される) ---
cbuffer PerMaterialBuffer : register(b1)
{
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float2 padding;
}

Texture2D objTexture : register(t0);
SamplerState objSamplerState : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 Normal : NORMAL;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

static const float PI = 3.14159265359f;

// --- TODO 4: フレネル関数 (Schlick's Approximation) ---
/*
    視線角度に依存する反射率の変化を計算します。
    引数:
    - cosTheta : (Normal と ViewDirection の内積、または Half と ViewDirection の内積等)
    - F0 : ベースとなる反射率
    
    要件:
    Schlick近似式 F = F0 + (1 - F0) * (1 - cosTheta)^5 を計算して返してください。
*/
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    // TODO: ここに実装を記述
    float3 F = F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
    return F;
}

// --- TODO 5: 法線分布関数 (Normal Distribution Function: GGX) ---
/*
    微小面の法線がハーフベクトル h にどの程度一致するかを計算します。
    引数:
    - NdotH : 法線ベクトル(N) と ハーフベクトル(H) の内積 
    - roughness : 粗さ設定

    要件:
    1. alpha = roughness * roughness とする。
    2. alphaSq = alpha * alpha とする。
    3. Trowbridge-Reitz GGX:
       D = alphaSq / (PI * (NdotH^2 * (alphaSq - 1.0) + 1.0)^2 )
       を計算して返してください。
*/
float DistributionGGX(float NdotH, float roughness)
{
    float alpha = pow(roughness, 2.0f);
    float alphaSq = pow(alpha, 2.0f);
    float D_h = alphaSq / (PI * pow(NdotH * NdotH * (alphaSq - 1.0f) + 1.0f, 2.0f));

    return D_h;
}

// 幾何減衰関数 (Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

// 幾何減衰関数合成 (Smith)
float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    // 各種ベクトルの計算
    float3 N = normalize(input.Normal);
    float3 V = normalize(CameraPos.xyz - input.WorldPos); // 視線ベクトル
    float3 L = normalize(-LightDir.xyz);                  // 光源ベクトル
    float3 H = normalize(V + L);                          // ハーフベクトル
    
    // ライトと法線、視線と法線の内積
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float HdotV = max(dot(H, V), 0.0f);

    // テクスチャのサンプリング
    float4 texColor = objTexture.Sample(objSamplerState, input.TexCoord);
    // sRGB -> Linear 空間への変換 (二重ガンマ補正を防止し、本来のテクスチャ色を復元)
    texColor.rgb = pow(max(texColor.rgb, 0.0f), 2.2f);
    
    // --- 【追加の穴埋め課題群】 ---
    
    // [TODO A] アルベドとベース反射率(F0)の計算
    /*
        非金属モデル（誘電体）のベース反射率(F0)は一般的に 0.04(4%) と定義されます。
        金属の反射は自身のアルベド(BaseColor)色になります。
        
        要件:
        1. albedo (float3)を、BaseColorのRGB成分とtexColorのRGB成分を掛け合わせて計算してください。
        2. F0 (float3)を float3(0.04) と、計算したalbedoの間で、Metallic値を使って線形補間(lerp)してください。
    */
    float3 albedo = float3(BaseColor.r * texColor.r, BaseColor.g * texColor.g, BaseColor.b * texColor.b);
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, Metallic);
    
    // [TODO B] BRDF 各項の計算
    /*
        要件:
        上で準備した各種関数(DistributionGGX, GeometrySmith, FresnelSchlick)を呼び出して、
        D, G, F の値を求めてください。
        注) FresnelSchlickの引数には HdotV または NdotV などを渡しますが、 Cook-Torranceでは HdotV を利用するのが一般的です。
    */
    float NDF = DistributionGGX(NdotH, Roughness);
    float G = GeometrySmith(NdotV, NdotL, Roughness);
    float3 F = FresnelSchlick(HdotV, F0);
    
    // [TODO C] Cook-Torrance の Specular 成分の計算
    /*
        要件:
        specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 0.0001); を計算してください。
        (0.0001はゼロ除算を防止するための微小値です)
    */
    float3 specular = (NDF * G * F) / (4.0f * NdotV * NdotL + 0.0001f);
    
    // [TODO D] エネルギー保存則に基づく Diffuse 色の決定
    /*
        PBRのエネルギー保存則にしたがい、光の反射(kS)から侵入や散乱の割合(kD)を決定します。
        
        要件:
        1. kS はフレネル関数(F)の結果そのものです。
        2. kD = (1.0 - kS) としてください。（1.0は float3(1.0, 1.0, 1.0)）
        3. 更に、金属にはDiffuse(散乱)成分がないため kD に (1.0 - Metallic) を掛け合わせてください。
    */
    float3 kS = F;
    float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - Metallic);
    
    // --- 最終計算 ---
    // ランバートディフューズ( albedo / PI ) と スペキュラ を足し合わせる
    float3 directLight = (kD * albedo / PI + specular) * LightColor.xyz * NdotL;
    
    // 環境光 (第12回の後半や以降でIBL拡張しますが、今回は単純な積)
    float3 ambientLight = Ambient.xyz * albedo;
    
    // 最終カラーの合成
    float3 finalColor = directLight + ambientLight;

    // トーンマッピング (Reinhard法: 1.0を超えるHDR光を圧縮して白飛びを防止)
    finalColor = finalColor / (finalColor + float3(1.0f, 1.0f, 1.0f));

    // ガンマ補正 (リニア空間 -> sRGB空間)
    finalColor = pow(max(finalColor, 0.0f), 1.0f / 2.2f);

    return float4(finalColor, BaseColor.a * texColor.a);
}
