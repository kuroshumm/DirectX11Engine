#include "DeadStateSystem.h"
#include "MathUtils.h"
#include "Components.h"

namespace Engine
{
    void DeadStateSystem::Update(entt::registry& registry, float dt)
    {
        const auto& view = registry.view<
            CharacterActionStateComponent,
            DamageResultComponent,
            AnimatorComponent,
            DeadStateComponent,
            CharacterActionResultComponent
        >();

        for (auto&& [entity, actionState, damageResult, animator, deadState, charaActionResult] : view.each())
        {
            if (actionState.currentState != ActionState::DEAD)
            {
                continue;
            }

            charaActionResult.velocity = { 0.0f, 0.0f, 0.0f };
            charaActionResult.shouldRotate = false;

            if (deadState.isActive)
            {
                deadState.elapsedTimer += dt;

                AnimSequenceAsset* pAnimSeq = animator.GetCurrentAnimSeq();
                float duration = (pAnimSeq && pAnimSeq->clip) ? pAnimSeq->clip->GetDurationInSeconds() : 0.0f;

                if (deadState.elapsedTimer >= duration + deadState.destroyTimer)
                {
                    deadState.isActive = false;
                }
            }

            if (damageResult.result == DamageResultType::DEAD)
            {
                deadState.isActive = true;
                deadState.elapsedTimer = 0.0f;

                charaActionResult.velocity = { 0.0f, 0.0f, 0.0f };
                charaActionResult.targetAnim = AnimStateType::DEATH;
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
