#include "LocomotionStateSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"
#include "Components/AnimationComponents.h"

namespace Engine
{
    void LocomotionStateSystem::Update(entt::registry& registry)
    {
        auto view = registry.view<
            CharacterActionStateComponent,
            LocomotionProposalComponent,
            CharacterMovementConfigComponent,
            LocomotionAnimComponent,
            CharacterLocomotionComponent,
            CharacterActionResultComponent
        >();

        for (auto&& [entity, charaActionState, locomotionProposal, moveConfig, locomotionAnim, charaLocomotion, charaActionResult] : view.each())
        {
            // 移動中でないなら何もしない
            if (charaActionState.currentState != ActionState::LOCOMOTION)
            {
                continue;
            }

            float speed = 0.0f;
            AnimStateType animName = AnimStateType::NONE;
            float fade = 0.25f;
            switch (locomotionProposal.state)
            {
                case LocomotionState::IDLE:
                    animName = AnimStateType::IDLE;
                    break;
                case LocomotionState::WALK:
                {
                    speed = moveConfig.walkSpeed;
                    fade = 0.20f;
                    size_t dirIndex = static_cast<size_t>(locomotionProposal.dir);
                    animName = locomotionAnim.pPreset->clips[dirIndex];
                    break;
                }
                case LocomotionState::RUN:
                {
                    speed = moveConfig.springSpeed;
                    animName = AnimStateType::RUN;
                    fade = 0.15f;
                    break;
                }
            }

            // アニメーション再生リクエストは一度だけ送るようにする
            if (charaLocomotion.currentState == locomotionProposal.state
                && !charaActionState.isChangeState)
            {
                animName = AnimStateType::NONE;
            }
            charaLocomotion.currentState = locomotionProposal.state;

             
            // アクション結果を出力
            charaActionResult.velocity = { locomotionProposal.velocity.x * speed, 0.0f, locomotionProposal.velocity.z * speed };
            charaActionResult.targetYaw = locomotionProposal.yaw;
            charaActionResult.shouldRotate = locomotionProposal.state != LocomotionState::IDLE;
            charaActionResult.turnSpeed = moveConfig.turnSpeed;
            charaActionResult.targetAnim = animName;
            charaActionResult.currentFadeDuration = fade;
        }
    }
}
