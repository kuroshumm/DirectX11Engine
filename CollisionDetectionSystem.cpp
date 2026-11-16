#include "CollisionDetectionSystem.h"
#include "Components.h"

namespace
{
    using namespace Engine;

    bool Collision(const WorldCollisionVolume& a, const WorldCollisionVolume& b)
    {
        return std::visit([](const auto& shapeA, const auto& shapeB) { return shapeA.Intersects(shapeB); }, a, b);
    }

    bool isActiveCollision(const DamageableCollisionComponent& dmgCol, const AttackCollisionComponent& atkCol,
                           const entt::entity dmgColEntity)
    {
        if (!dmgCol.isActive)
        {
            return false;
        }
        if (atkCol.owner == dmgColEntity)
        {
            return false;
        }
        if (atkCol.hitEntities.contains(dmgColEntity))
        {
            return false;
        }

        if (!Collision(atkCol.worldVolume, dmgCol.worldVolume))
        {
            return false;
        }

        return true;
    }
} // namespace

namespace Engine
{
    void CollisionDetectionSystem::Update(entt::registry& registry)
    {
        const auto& atkColView = registry.view<AttackCollisionComponent>();
        const auto& dmgColView = registry.view<DamageableCollisionComponent, DamageRequestComponent>();

        for (auto&& [atkColEntity, atkCol] : atkColView.each())
        {
            for (auto&& [dmgColEntity, dmgCol, damageReq] : dmgColView.each())
            {
                if (!isActiveCollision(dmgCol, atkCol, dmgColEntity))
                {
                    continue;
                }

                atkCol.hitEntities.emplace(dmgColEntity);

                damageReq.hasPendingDamage = true;
                damageReq.attackerEntity = atkCol.owner;
                damageReq.damage = atkCol.damage;

            }
        }
    }
} // namespace Engine
