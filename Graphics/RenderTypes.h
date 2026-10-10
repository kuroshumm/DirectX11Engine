#pragma once
#include <DirectXMath.h>

namespace Engine
{
    // Vertex data structure
    struct Vertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent;
        DirectX::XMFLOAT4 Color;
        DirectX::XMFLOAT2 TexCoord;
    };

    // Constant buffer data for scene (16-byte aligned)
    __declspec(align(16)) struct ConstantBufferData
    {
        DirectX::XMMATRIX mWorld;
        DirectX::XMMATRIX mView;
        DirectX::XMMATRIX mProjection;
        DirectX::XMFLOAT4 vLightDir;
        DirectX::XMFLOAT4 vLightColor;
        DirectX::XMFLOAT4 vAmbient;
        DirectX::XMFLOAT4 vCameraPos;
    };

    // Constant buffer data for material (16-byte aligned)
    __declspec(align(16)) struct MaterialConstantBufferData
    {
        DirectX::XMFLOAT4 BaseColor;
        float Metallic;
        float Roughness;
        DirectX::XMFLOAT2 padding;
    };

    enum class PrimitiveType
    {
        Plane,
        Cube,
        Sphere,
        Cylinder
    };
}
