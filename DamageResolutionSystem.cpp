#include "DamageResolutionSystem.h"
#include "Components.h"

namespace
{
    using namespace Engine;

    DamageResultType CalcDamageResult(const HealthComponent& health)
    {
        if (health.hp <= 0)
        {
            return DamageResultType::DEAD;
        }

        return DamageResultType::DAMAGED;
    }
}

namespace Engine
{
    void DamageResolutionSystem::Update(entt::registry& registry)
    {
        const auto& view = registry.view<
            DamageableCollisionComponent,
            DamageRequestComponent,
            DamageResultComponent,
            HealthComponent
        >();


        for (auto&& [entity, damageableCol, damageReq, damageResult, health] : view.each())
        {
            damageResult.result = DamageResultType::NONE;

            // ダメージ要求がないなら何もしない
            if (!damageReq.hasPendingDamage)
            {
                continue;
            }
            damageReq.hasPendingDamage = false;

            // ダメージをHPに反映
            health.hp = std::clamp(health.hp - damageReq.damage, 0, health.maxHp);

            // 死亡時は以降の被弾判定を無効化
            if (health.hp <= 0)
            {
                damageableCol.isActive = false;
            }

            // 
            damageResult.result = CalcDamageResult(health);
            damageResult.attackerEntity = damageReq.attackerEntity;
        }
    }
}
