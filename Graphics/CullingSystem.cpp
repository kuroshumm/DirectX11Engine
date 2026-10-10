#include "CullingSystem.h"

namespace
{
    using namespace Engine;

    entt::entity FindActiveCameraEntity(entt::registry& registry)
    {
        auto cameraView = registry.view<TransformComponent, CameraComponent, ActiveCameraTag>();
        if (cameraView.each().begin() == cameraView.each().end())
        {
            return entt::null;
        }
        auto cameraEntity = cameraView.front();

        return cameraEntity;
    }

    bool CheckFrustum(TransformComponent& transform, ModelComponent& model, const Frustum& frustum)
    {
        DirectX::XMFLOAT3 center = transform.position;
        float localRadius = model.pModelAsset->totalLocalBounds.Radius;
        float maxScale = (std::max)({ transform.scale.x, transform.scale.y, transform.scale.z });
        float worldRadius = localRadius * maxScale;

        // 視錐台カリング判定
        return frustum.CheckSphere(center, worldRadius);
    }

    void UpdateLOD(
        entt::registry& registry,
        entt::entity entity,
        TransformComponent& transformComp,
        DirectX::XMVECTOR cameraPos
    )
    {
        // LODコンポーネントを持つエンティティのみLOD更新
        LODComponent* lod = registry.try_get<LODComponent>(entity);
        if (!lod)
        {
            // LODコンポーネントを持たないエンティティは毎フレーム更新
            if (!registry.all_of<AnimationUpdateTag>(entity))
            {
                registry.emplace<AnimationUpdateTag>(entity);
            }
            return;
        }

        // カメラとエンティティとの距離
        DirectX::XMVECTOR entityPos = DirectX::XMLoadFloat3(&transformComp.position);
        float distanceToCamera = DirectX::XMVectorGetX(
            DirectX::XMVector3Length(DirectX::XMVectorSubtract(entityPos, cameraPos))
        );
        lod->distanceToCamera = distanceToCamera;

        // 距離 → LODレベル → 更新間隔
        if (distanceToCamera < 20.0f) 
        {
            lod->level = 0; 
            lod->updateInterval = 1; 
        }
        else if (distanceToCamera < 50.0f) 
        {
            lod->level = 1; 
            lod->updateInterval = 3; 
        }
        else 
        {
            lod->level = 2; 
            lod->updateInterval = 6; 
        }

        lod->frameCounter++;
        if (lod->frameCounter >= lod->updateInterval)
        {
            lod->frameCounter = 0;
            // 更新フレーム：タグを付与
            if (!registry.all_of<AnimationUpdateTag>(entity))
            {
                registry.emplace<AnimationUpdateTag>(entity);
            }
        }
        else
        {
            // 間引きフレーム：タグを削除
            registry.remove<AnimationUpdateTag>(entity);
        }
    }
}

namespace Engine
{
	void CullingSystem::Update(entt::registry& registry)
	{
        // アクティブなカメラを取得
        auto cameraEntity = FindActiveCameraEntity(registry);
        if (cameraEntity == entt::null)
        {
            return;
        }
        const auto& camComponent = registry.get<CameraComponent>(cameraEntity);
        const auto& camTransform = registry.get<TransformComponent>(cameraEntity);

        // カメラの視錐台を取得
        const Frustum& frustum = camComponent.frustum;
        // カメラの座標
        DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat3(&camTransform.position);

        auto view = registry.view<TransformComponent, ModelComponent>();
        for (auto [entity, transformComp, modelComp] : view.each())
        {
            // 画面内にいない場合はタグを削除
            if (!CheckFrustum(transformComp, modelComp, frustum))
            {
                registry.remove<VisibleTag>(entity);
                registry.remove<AnimationUpdateTag>(entity);
                continue;
            }

            // 画面内にいる場合 & 付与されてない場合にタグを付与
            if (!registry.all_of<VisibleTag>(entity))
            {
                registry.emplace<VisibleTag>(entity);
            }
            
            // LOD情報の更新
            UpdateLOD(registry, entity, transformComp, cameraPos);
        }
	}
}