#pragma once
#include <memory>
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include "ModelAsset.h"
#include "Texture.h"
#include "Shader.h"

namespace Engine
{
    struct ModelComponent
    {
        std::shared_ptr<const ModelAsset> pModelAsset = nullptr;
    };

    struct MaterialComponent
    {
        std::shared_ptr<Texture> pTexture;
        std::shared_ptr<Shader>  pShader;
        DirectX::XMFLOAT4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float metallic = 0.0f;
        float roughness = 0.5f;
    };

    struct SkinningPaletteComponent
    {
        Microsoft::WRL::ComPtr<ID3D11Buffer> pPaletteBuffer;
    };

    struct LODComponent
    {
        int   level = 0;
        int   updateInterval = 0;
        int   frameCounter = 0;
        float distanceToCamera = 0.0f;
    };

    struct VisibleTag {};
}
