#include "CollisionVisualizerSystem.h"
#include "Components.h"
#include "PrimitiveFactory.h"
#include "CollisionType.h"

namespace
{
    using namespace Engine;

    void CreateVisualPrimitive(
        entt::registry& registry,
        const entt::entity ownerCollision,
        const WorldCollisionVolume& worldVolume
    )
    {
        entt::entity visEntity = entt::null;

        if (const auto* pSphere = std::get_if<DirectX::BoundingSphere>(&worldVolume))
        {
            const float diameter = pSphere->Radius * 2.0f;

            visEntity = PrimitiveFactory::CreatePrimitive(
                registry,
                PrimitiveType::Sphere,
                pSphere->Center,
                DirectX::XMFLOAT3(diameter, diameter, diameter),
                DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
                DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.2f)
            );
        }
        if (const auto* pBox = std::get_if<DirectX::BoundingOrientedBox>(&worldVolume))
        {
            const DirectX::XMFLOAT3 fullScale(
                pBox->Extents.x * 2.0f,
                pBox->Extents.y * 2.0f,
                pBox->Extents.z * 2.0f
            );

            visEntity = PrimitiveFactory::CreatePrimitive(
                registry,
                PrimitiveType::Cube,
                pBox->Center,
                fullScale,
                pBox->Orientation,
                DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.2f)
            );
        }

        if (visEntity != entt::null)
        {
            // コリジョンに可視化タグを付与
            registry.emplace<CollisionVisualizeTag>(ownerCollision);

            // 可視化してるコリジョンの情報を付与
            CollisionVisualizeComponent colVisComp;
            colVisComp.ownerCollision = ownerCollision;
            registry.emplace<CollisionVisualizeComponent>(visEntity, colVisComp);
        }
    }

    void SyncPrimitiveTransform(
        TransformComponent& primTransform,
        const WorldCollisionVolume& worldVolume
    )
    {
        if (const auto* pSphere = std::get_if<DirectX::BoundingSphere>(&worldVolume))
        {
            primTransform.position = pSphere->Center;
        }
        if (const auto* pBox = std::get_if<DirectX::BoundingOrientedBox>(&worldVolume))
        {
            primTransform.position = pBox->Center;
            primTransform.rotation = pBox->Orientation;
        }
    }
    
}

namespace Engine
{
    void CollisionVisualizerSystem::Update(entt::registry& registry)
    {
        // まだ可視化してないコリジョンに対して可視化用のプリミティブを生成
        auto atkColView = registry.view<AttackCollisionComponent>(entt::exclude<CollisionVisualizeTag>);
        for (auto [entity, atkCol] : atkColView.each())
        {
            CreateVisualPrimitive(registry, entity, atkCol.worldVolume);
        }

        auto dmgColView = registry.view<DamageableCollisionComponent>(entt::exclude<CollisionVisualizeTag>);
        for (auto [entity, dmgCol] : dmgColView.each())
        {
            CreateVisualPrimitive(registry, entity, dmgCol.worldVolume);
        }

        // 2. 既存の可視化プリミティブの座標同期・および親コリジョン消滅時の自動破棄
        std::vector<entt::entity> destroyList;

        auto visView = registry.view<CollisionVisualizeComponent, TransformComponent>();
        for (auto [visEntity, visComp, visTransform] : visView.each())
        {
            // 追跡対象のコリジョンエンティティが CollisionSpawnSystem 等によって破棄されていたら、自身も破棄リストへ
            if (!registry.valid(visComp.ownerCollision)
                || (!registry.all_of<AttackCollisionComponent>(visComp.ownerCollision)
                && !registry.all_of<DamageableCollisionComponent>(visComp.ownerCollision))
            )
            {
                destroyList.push_back(visEntity);
                continue;
            }

            // 生存していれば最新の worldVolume に合わせてプリミティブの Transform を更新
            const auto* attackCol = registry.try_get<AttackCollisionComponent>(visComp.ownerCollision);
            if (attackCol)
            {
                SyncPrimitiveTransform(visTransform, attackCol->worldVolume);
            }
            const auto* dmgCol = registry.try_get<DamageableCollisionComponent>(visComp.ownerCollision);
            if (dmgCol)
            {
                SyncPrimitiveTransform(visTransform, dmgCol->worldVolume);
            }
        }

        for (entt::entity visEntity : destroyList)
        {
            registry.destroy(visEntity);
        }
    }
}
