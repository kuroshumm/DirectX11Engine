#pragma once
#include <memory>
#include <entt/entt.hpp>

#include "Graphics.h"
#include "Shader.h"
#include "Texture.h"
#include "Common.h"
#include "Components.h"
#include "PrimitiveMeshCache.h"

namespace Engine
{
    class PrimitiveFactory
    {
    public:
        static void Initialize(
            Graphics* pGfx,
            std::shared_ptr<Shader> pStaticShader,
            std::shared_ptr<Texture> pDefaultTexture = nullptr
        );
        
        static entt::entity CreatePrimitive(
            entt::registry& registry,
            PrimitiveType type,
            const DirectX::XMFLOAT3& position = { 0.0f, 0.0f, 0.0f },
            const DirectX::XMFLOAT3& scale = { 1.0f, 1.0f, 1.0f },
            const DirectX::XMFLOAT4& baseColor = { 1.0f, 1.0f, 1.0f, 1.0f }
        );

    private:
        inline static Graphics* m_pGfx = nullptr;
        inline static std::shared_ptr<Shader> m_staticShader = nullptr;
        inline static std::shared_ptr<Texture> m_defaultTexture = nullptr;
        inline static PrimitiveMeshCache m_meshCache; // 形状ごとの ModelAsset キャッシュ

    };
}
