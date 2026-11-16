#pragma once
#include <entt/entt.hpp>

#include "Model/ModelLoader.h"

namespace Engine
{
    class Graphics;

    struct HumanoidSpawnDesc
    {
        std::shared_ptr<Shader> pShader = nullptr;
        std::shared_ptr<Texture> pTexture = nullptr;
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 scale = { 0.01f, 0.01f, 0.01f };
    };

    class CharacterFactory
    {
    public:
        CharacterFactory(
            ID3D11Device* pDevice,
            entt::registry& registry,
            ModelLoader& modelLoader
        );

        // モデルアセットのロード
        HRESULT LoadModelAsset();

        // 操作プレイヤーの生成
        entt::entity SpawnHumanoid(const HumanoidSpawnDesc& desc = {});

    private:
        ID3D11Device* m_pDevice;

        entt::registry& m_registry;
        ModelLoader& m_modelLoader;

        ModelLoader::LoadResult m_loadResult;
        std::shared_ptr<Texture> m_pDefaultTexture;
    };
}
