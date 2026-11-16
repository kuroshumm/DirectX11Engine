#include "RenderSystem.h"
#include "Components.h"

namespace
{
    using namespace Engine;

    CameraComponent* FindActiveCamera(entt::registry& registry)
    {
        auto view = registry.view<CameraComponent, ActiveCameraTag>();
        if (view.each().begin() == view.each().end())
        {
            return nullptr;
        }
        auto entity = view.front();
        return &registry.get<CameraComponent>(entity);
    }

    void UpdateWorldMat(TransformComponent& transform, Graphics& gfx, DirectX::XMMATRIX& viewMat, DirectX::XMMATRIX& projMat)
    {
        DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&transform.worldMatrix);
        gfx.Update(worldMat, viewMat, projMat);
    }

    void RenderMesh(ModelComponent& model, ID3D11DeviceContext& context)
    {
        for (const auto& subMesh : model.pModelAsset->subMeshs)
        {
            UINT stride = subMesh.vertexStride;
            UINT offset = 0;

            // 頂点バッファのバインド
            context.IASetVertexBuffers(0, 1, subMesh.vertexBuffer.GetAddressOf(), &stride, &offset);
            // インデックスバッファのバインド
            context.IASetIndexBuffer(subMesh.indexBuffer.Get(), DXGI_FORMAT_R16_UINT, offset);
            // インデックス指定による描画実行命令
            context.DrawIndexed(subMesh.indexCount, 0, 0);
        }
    }
} // namespace

namespace Engine
{
    HRESULT RenderSystem::Initialize(ID3D11Device* pDevice)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(MaterialConstantBufferData);
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pMaterialCB);

        return hr;
    }

    void RenderSystem::Render(entt::registry& registry, Graphics& gfx)
    {
        // 1. アクティブカメラの取得 (.each() による安全な走査)
        CameraComponent* pActiveCamera = FindActiveCamera(registry);
        if (pActiveCamera == nullptr)
        {
            return;
        }

        // カメラ行列と視錐台の取得
        DirectX::XMMATRIX viewMat = DirectX::XMLoadFloat4x4(&pActiveCamera->viewMatrix);
        DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&pActiveCamera->projectionMatrix);

        ID3D11DeviceContext* pContext = gfx.GetContext();

        // --- パス1: 不透明オブジェクトの描画 (ブレンド無効・深度書き込み有効) ---
        gfx.DisableAlphaBlend();

        // 1-A. 不透明な静的メッシュ (スキンモデルおよび半透明コリジョン可視化プリミティブを除外)
        auto staticEntities = registry.view<TransformComponent, ModelComponent, MaterialComponent, VisibleTag>(
            entt::exclude<SkeletonComponent, CollisionVisualizeComponent>);

        for (auto [entity, transform, model, material] : staticEntities.each())
        {
            UpdateWorldMat(transform, gfx, viewMat, projMat);
            UpdateMaterial(material, *pContext);
            RenderMesh(model, *pContext);
        }

        // 1-B. スキンモデル (キャラクター)
        auto skinnedEntities =
            registry.view<TransformComponent, ModelComponent, MaterialComponent, SkeletonComponent, SkinningPaletteComponent, VisibleTag>();

        for (auto [entity, transform, model, material, skelton, palette] : skinnedEntities.each())
        {
            UpdateWorldMat(transform, gfx, viewMat, projMat);
            UpdateMaterial(material, *pContext);

            if (palette.pPaletteBuffer)
            {
                pContext->VSSetConstantBuffers(1, 1, palette.pPaletteBuffer.GetAddressOf());
            }

            RenderMesh(model, *pContext);
        }

        // --- パス2: 半透明コリジョン可視化プリミティブの描画 (アルファブレンド有効・深度書き込み無効) ---
        gfx.EnableAlphaBlend();

        auto transparentVisEntities =
            registry.view<TransformComponent, ModelComponent, MaterialComponent, CollisionVisualizeComponent, VisibleTag>();

        for (auto [entity, transform, model, material, colVis] : transparentVisEntities.each())
        {
            UpdateWorldMat(transform, gfx, viewMat, projMat);
            UpdateMaterial(material, *pContext);
            RenderMesh(model, *pContext);
        }

        // ステートを不透明用へ復帰
        gfx.DisableAlphaBlend();
    }

    void RenderSystem::UpdateMaterial(MaterialComponent& material, ID3D11DeviceContext& context)
    {
        // マテリアルのバインド
        material.pShader->Bind(&context);

        // テクスチャのバインド
        if (material.pTexture)
        {
            material.pTexture->Bind(&context, 0);
        }

        // マテリアル定数バッファの更新とバインド
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(context.Map(m_pMaterialCB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        {
            MaterialConstantBufferData* data = static_cast<MaterialConstantBufferData*>(mapped.pData);
            data->BaseColor = material.baseColor;
            data->Metallic = material.metallic;
            data->Roughness = material.roughness;
            context.Unmap(m_pMaterialCB.Get(), 0);
            // ピクセルシェーダーの b1 スロットへバインド
            context.PSSetConstantBuffers(1, 1, m_pMaterialCB.GetAddressOf());
        }
    }
} // namespace Engine
