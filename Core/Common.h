#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

namespace Engine
{
    constexpr int MAX_BONES = 256;

    // 頂点データ構造 (第8回時点)
    struct Vertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent; // 追加: 12バイト
        DirectX::XMFLOAT4 Color;
        DirectX::XMFLOAT2 TexCoord;
    };

    // 定数バッファ構造 (第8回時点: 16バイトアライメント)
    __declspec(align(16)) struct ConstantBufferData
    {
        DirectX::XMMATRIX mWorld;
        DirectX::XMMATRIX mView;
        DirectX::XMMATRIX mProjection;
        DirectX::XMFLOAT4 vLightDir;
        DirectX::XMFLOAT4 vLightColor;
        DirectX::XMFLOAT4 vAmbient;
        DirectX::XMFLOAT4 vCameraPos; // 第12回追加: 視線ベクトル計算用
    };

    // 第12回追加: マテリアル用定数バッファ構造
    __declspec(align(16)) struct MaterialConstantBufferData
    {
        DirectX::XMFLOAT4 BaseColor;
        float Metallic;
        float Roughness;
        DirectX::XMFLOAT2 padding; // 16バイトアライメントのためのパディング
    };

    // スキニング用頂点構造体
    struct SkinnedVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent; // 追加: 12バイト
        DirectX::XMFLOAT4 Color;
        DirectX::XMFLOAT2 TexCoord;
        DirectX::XMUINT4  BlendIndices;
        DirectX::XMFLOAT4 BlendWeight;
    };

    // ボーン用構造体
    struct Bone
    {
        // このボーンの親となるボーンのインデックス（親がいないルートボーンの場合は -1）
        int                 parentIndex;
        // このボーンの子となるボーンのインデックス配列
        std::vector<int>    childIndices;
        // デバッグや辞書検索に用いるボーン名（Assimp側の mName と完全に一致します）
        std::string         name;
        //
        int                 animChannelIndex = -1;
        // 初期ポーズ（バインドポーズ）における、このボーンの「逆行列」
        DirectX::XMFLOAT4X4 InverseBindMatrix;
        // 【ローカル行列】親のボーンの関節から見た、自分自身の相対的な位置・回転・スケール
        DirectX::XMFLOAT4X4 LocalTransformMatrix;
        // 【グローバル行列】モデルの原点（足元）から見た、自分自身の絶対的な位置・回転・スケール
        DirectX::XMFLOAT4X4 GlobalTransformMatrix;

        // ロード時の初期姿勢を保存
        DirectX::XMFLOAT3   defaultPos;
        DirectX::XMFLOAT4   defaultRot;
        DirectX::XMFLOAT3   defaultScale;
    };

    // 行列用パレット用定数バッファ
    __declspec(align(16)) struct PaletteBufferData
    {
        DirectX::XMMATRIX Bones[MAX_BONES];
    };

    // 位置とスケール用
    struct KeyVector3
    {
        float Time;
        DirectX::XMFLOAT3 Value;
    };
    // 回転用
    struct KeyQuaternion
    {
        float Time;
        DirectX::XMFLOAT4 Value;
    };
    // 一つのボーンが持つ時間順にソートされたキーフレーム
    struct NodeAnimation
    {
        std::vector<KeyVector3> ScalingKeys;
        std::vector<KeyVector3> PositionKeys;
        std::vector<KeyQuaternion> RotaionKeys;
    };

    enum class PrimitiveType
    {
        Plane,
        Cube,
        Sphere,
        Cylinder
    };
}
