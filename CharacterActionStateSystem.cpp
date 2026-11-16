#include "CharacterActionStateSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"

namespace
{
    using namespace Engine;

    // 状態遷移の共通手続き
    void ChangeState(CharacterActionStateComponent& actionState, ActionState nextState)
    {
        actionState.currentState = nextState;
        actionState.stateTimer = 0.0f;
        actionState.isChangeState = true;
    }

    // 攻撃中断・要求破棄のクリーンアップ
    void CancelCombatState(CombatRequestComponent& combatReq, CharacterCombatStateComponent& combatState)
    {
        combatReq.isExecuted = true;
        combatState.activeAttackId = CombatAttackID::NONE;
        combatState.attackTimer = 0.0f;
        combatState.hasQueuedCombo = false;
        combatState.isHitRegistered = false;
    }

    bool IsChangeAttackState(CombatRequestComponent& combatReq, ActionState currentState)
    {
        // 攻撃要求があるか
        bool hasCombatRequest = !combatReq.isExecuted
            && combatReq.intent != CombatInputIntent::NONE;
        if (!hasCombatRequest)
        {
            return false;
        }

        // 現在のモーションがLocomotion
        if (currentState != ActionState::LOCOMOTION)
        {
            return false;
        }

        return true;
    }
}

namespace Engine
{
    void CharacterActionStateSystem::Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<
            LocomotionProposalComponent,
            CombatRequestComponent,
            DamageResultComponent,
            CharacterActionStateComponent,
            CharacterCombatStateComponent,
            DamageStateComponent
        >();

        for (auto&& [entity, locomotionProposal, combatReq, damageResult, charaActionState, charaCombatState, damageState] : view.each())
        {
            charaActionState.isChangeState = false;

            // ステートの滞在時間更新
            charaActionState.stateTimer += dt;

            // 死亡状態は終端状態として他状態への上書き・復帰を禁止
            if (charaActionState.currentState == ActionState::DEAD)
            {
                continue;
            }

            // 死亡への遷移
            if (damageResult.result == DamageResultType::DEAD)
            {
                CancelCombatState(combatReq, charaCombatState);
                ChangeState(charaActionState, ActionState::DEAD);
                continue;
            }
            // ダメージへの遷移
            if (damageResult.result == DamageResultType::DAMAGED)
            {
                CancelCombatState(combatReq, charaCombatState);
                ChangeState(charaActionState, ActionState::DAMAGED);
                continue;
            }
            // 攻撃への遷移
            if (IsChangeAttackState(combatReq, charaActionState.currentState))
            {
                ChangeState(charaActionState, ActionState::ATTACK);
                continue;
            }

            // Locomotionへの戻し
            switch (charaActionState.currentState)
            {
            case ActionState::ATTACK:
                // 攻撃実行システム側で技が終了した場合、通常移動へ復帰
                if (charaCombatState.activeAttackId == CombatAttackID::NONE)
                {
                    ChangeState(charaActionState, ActionState::LOCOMOTION);
                }
                break;
            case ActionState::DAMAGED:
                if (!damageState.isActive)
                {
                    ChangeState(charaActionState, ActionState::LOCOMOTION);
                }
                break;
            case ActionState::DODGE:
                break;
            }

        }
    }
}
