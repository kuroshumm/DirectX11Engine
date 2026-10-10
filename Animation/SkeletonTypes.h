#pragma once
#include <DirectXMath.h>
#include <string>
#include <vector>

namespace Engine
{
    constexpr int MAX_BONES = 256;

    // Skinned vertex structure
    struct SkinnedVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent;
        DirectX::XMFLOAT4 Color;
        DirectX::XMFLOAT2 TexCoord;
        DirectX::XMUINT4  BlendIndices;
        DirectX::XMFLOAT4 BlendWeight;
    };

    // Bone structure
    struct Bone
    {
        // Parent bone index (-1 for root)
        int                 parentIndex = -1;
        // Child bone indices
        std::vector<int>    childIndices;
        // Bone name (matches Assimp mName)
        std::string         name;
        // Animation channel index
        int                 animChannelIndex = -1;
        // Inverse bind matrix
        DirectX::XMFLOAT4X4 InverseBindMatrix;
        // Local transform matrix
        DirectX::XMFLOAT4X4 LocalTransformMatrix;
        // Global transform matrix
        DirectX::XMFLOAT4X4 GlobalTransformMatrix;

        // Default rest pose
        DirectX::XMFLOAT3   defaultPos;
        DirectX::XMFLOAT4   defaultRot;
        DirectX::XMFLOAT3   defaultScale;
    };

    // Bone palette constant buffer (16-byte aligned)
    __declspec(align(16)) struct PaletteBufferData
    {
        DirectX::XMMATRIX Bones[MAX_BONES];
    };

    // Keyframe for vector3 (position, scale)
    struct KeyVector3
    {
        float Time;
        DirectX::XMFLOAT3 Value;
    };

    // Keyframe for quaternion (rotation)
    struct KeyQuaternion
    {
        float Time;
        DirectX::XMFLOAT4 Value;
    };

    // Node animation channel data
    struct NodeAnimation
    {
        std::vector<KeyVector3> ScalingKeys;
        std::vector<KeyVector3> PositionKeys;
        std::vector<KeyQuaternion> RotaionKeys;
    };
}
