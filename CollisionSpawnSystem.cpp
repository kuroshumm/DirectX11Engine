#include "CollisionSpawnSystem.h"
#include "CollisionType.h"

namespace
{
    using namespace Engine;

    WorldCollisionVolume CalcWorldColVolume(
        const TransformComponent& ownerTransform,
        const DirectX::XMFLOAT3& offset,
        const CollisionShapeDesc& shapeDesc
    )
    {
        DirectX::XMVECTOR vOwnerPos = DirectX::XMLoadFloat3(&ownerTransform.position);
        DirectX::XMVECTOR vOwnerRot = DirectX::XMLoadFloat4(&ownerTransform.rotation);
        DirectX::XMVECTOR vOwnerScale = DirectX::XMLoadFloat3(&ownerTransform.scale);

        DirectX::XMVECTOR vOffset = DirectX::XMLoadFloat3(&offset);
        DirectX::XMVECTOR vRotatedOffset = DirectX::XMVector3Rotate(vOffset, vOwnerRot);
        DirectX::XMVECTOR vCenter = DirectX::XMVectorAdd(vOwnerPos, vRotatedOffset);

        DirectX::XMFLOAT3 center{};
        DirectX::XMStoreFloat3(&center, vCenter);

        if (const auto* pSphere = std::get_if<SphereShapeDesc>(&shapeDesc))
        {
            DirectX::BoundingSphere sphere;
            sphere.Center = center;
            sphere.Radius = pSphere->radius;
            return sphere;
        }
        if (const auto* pBox = std::get_if<BoxShapeDesc>(&shapeDesc))
        {
            DirectX::BoundingOrientedBox obb;
            obb.Center = center;
            obb.Extents = pBox->halfExtents;
            obb.Orientation = ownerTransform.rotation;
            return obb;
        }

        return DirectX::BoundingSphere{};
    }

    void SpawnAttackCollision(entt::registry& registry)
    {
        auto animEventView = registry.view<
            AnimEventComponent,
            AnimatorComponent,
            CharacterCombatStateComponent,
            CombatAttackPresetComponent
        >();

        for (auto [entity, animEvent, animator, charaCombatState, combatAttackPreset] : animEventView.each())
        {
            // 攻撃コリジョンが有効になったか
            bool isEnterAttackCol = IsEventWindowEntered(animEvent, animator.currentState, AnimEventWindowType::ATTACK_COLLISION);
            if (!isEnterAttackCol)
            {
                continue;
            }

            // コリジョン生成したキャラのTransform取得
            const TransformComponent& transform = registry.get<TransformComponent>(entity);

            // 攻撃データを取得
            int attackID = static_cast<int>(charaCombatState.activeAttackId);
            const CombatAttackDesc& desc = combatAttackPreset.pPreset->attacks[attackID];

            // 攻撃コリジョンデータを生成
            entt::entity colEntity = registry.create();
            AttackCollisionComponent attackCol{};
            attackCol.owner = entity;
            attackCol.sourceState = animator.currentState;
            attackCol.localOffset = desc.collisionOffset;
            attackCol.shapeDesc = desc.collisionShape;
            attackCol.worldVolume = CalcWorldColVolume(transform, desc.collisionOffset, desc.collisionShape);
            attackCol.damage = desc.damage;

            // 攻撃コリジョンのエンティティを登録
            registry.emplace<AttackCollisionComponent>(colEntity, attackCol);
        }
    }

    void UpdateAttackCollision(entt::registry& registry)
    {
        std::vector<entt::entity> destroyList;

        auto attackColView = registry.view<AttackCollisionComponent>();
        for (auto&& [entity, attackCol] : attackColView.each())
        {
            // コリジョンが破棄されるか判定
            const auto& ownerAnimEvent = registry.get<AnimEventComponent>(attackCol.owner);
            if (IsEventWindowExited(ownerAnimEvent, attackCol.sourceState, AnimEventWindowType::ATTACK_COLLISION))
            {
                destroyList.push_back(entity);
                continue;
            }

            // コリジョンの座標更新
            const auto& ownerTransform = registry.get<TransformComponent>(attackCol.owner);
            attackCol.worldVolume = CalcWorldColVolume(ownerTransform, attackCol.localOffset, attackCol.shapeDesc);
        }

        // 攻撃コリジョンの破棄
        for (auto colEntity : destroyList)
        {
            registry.destroy(colEntity);
        }
    }
}

namespace Engine
{
    void CollisionSpawnSystem::Update(entt::registry& registry)
    {
        // 攻撃コリジョン更新（座標と破棄）
        UpdateAttackCollision(registry);

        // 攻撃コリジョン生成
        SpawnAttackCollision(registry);

        // 被弾側のコリジョンの座標更新
        auto damageableView = registry.view<TransformComponent, DamageableCollisionComponent>();
        for (auto&& [entity, transform, damageableCol] : damageableView.each())
        {
            if (!damageableCol.isActive)
            {
                continue;
            }
            damageableCol.worldVolume = CalcWorldColVolume(transform, damageableCol.localOffset, damageableCol.shapeDesc);
        }
    }
}
