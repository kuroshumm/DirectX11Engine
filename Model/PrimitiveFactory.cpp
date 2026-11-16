#include "PrimitiveFactory.h"

#include "GeometryGenerator.h"

namespace Engine
{
    void PrimitiveFactory::Initialize(
        Graphics* pGfx,
        std::shared_ptr<Shader> pStaticShader,
        std::shared_ptr<Texture> pDefaultTexture
    )
    {
        m_pGfx = pGfx;
        m_staticShader = pStaticShader;

        if (pDefaultTexture == nullptr)
        {
            m_defaultTexture = std::make_shared<Texture>();
            m_defaultTexture->CreateSolidColor(m_pGfx->GetDevice());
            return;
        }
        m_defaultTexture = pDefaultTexture;
    }

    entt::entity PrimitiveFactory::CreatePrimitive(
        entt::registry& registry,
        PrimitiveType type,
        const DirectX::XMFLOAT3& position,
        const DirectX::XMFLOAT3& scale,
        const DirectX::XMFLOAT4& rot,
        const DirectX::XMFLOAT4& baseColor
    )
    {
        // Entityを作成
        entt::entity entity = registry.create();

        // TransformComponent作成
        TransformComponent transformComp;
        transformComp.position = position;
        transformComp.scale = scale;
        transformComp.rotation = rot;
        registry.emplace<TransformComponent>(entity, transformComp);

        // ModelAssetを作成
        ID3D11Device* pDevice = m_pGfx->GetDevice();
        std::shared_ptr<ModelAsset> pModelAsset =m_meshCache.GetOrCreate(pDevice, type);
        registry.emplace<ModelComponent>(entity, pModelAsset);

        // MaterialComponent
        MaterialComponent materialComp;
        materialComp.pShader = m_staticShader;
        materialComp.pTexture = m_defaultTexture;
        materialComp.baseColor = baseColor;
        registry.emplace<MaterialComponent>(entity, materialComp);

        return entity;
    }
}
