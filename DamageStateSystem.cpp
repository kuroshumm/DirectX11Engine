#include "DamageStateSystem.h"
#include "Components.h"
#include "MathUtils.h"
#include "DebugLog.h"

namespace Engine
{
    void DamageStateSystem::Update(entt::registry& registry, float dt)
    {
        const auto& view = registry.view<
            CharacterActionStateComponent,
            AnimatorComponent,
            AnimEventComponent,
            DamageStateComponent,
            DamageResultComponent,
            CharacterActionResultComponent
        >();

        for (auto&& [entity, charaActionState, animator, animEvent, damageState, damageResult, charaActionResult] : view.each())
        {
            if (charaActionState.currentState != ActionState::DAMAGED)
            {
                continue;
            }

            if (damageState.isActive)
            {
                damageState.damageTimer += dt;

                AnimSequenceAsset* pAnimSeq = animator.GetCurrentAnimSeq();
                float duration = (pAnimSeq && pAnimSeq->clip) ? pAnimSeq->clip->GetDurationInSeconds() : 0.0f;

                if (damageState.damageTimer >= duration)
                {
                    damageState.isActive = false;
                }
            }

            if (damageResult.result == DamageResultType::DAMAGED)
            {
                damageState.isActive = true;
                damageState.damageTimer = 0.0f;

                charaActionResult.velocity = { 0.0f, 0.0f, 0.0f };
                charaActionResult.targetAnim = AnimStateType::DAMAGE;
                charaActionResult.currentFadeDuration = 0.05f;

                if (registry.valid(damageResult.attackerEntity))
                {
                    const auto& damagedTransform = registry.get<TransformComponent>(entity);
                    const auto& attackTransform = registry.get<TransformComponent>(damageResult.attackerEntity);
                    charaActionResult.targetYaw = Math::CalcLookAtYaw(damagedTransform.position, attackTransform.position);
                    charaActionResult.shouldRotate = true;
                }
            }
            
        }
    }
}
