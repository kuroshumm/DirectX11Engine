#include "AttackingStateSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"
#include "Components/AnimationComponents.h"
#include "Components/TransformComponents.h"
#include "MathUtils.h"

namespace
{
    using namespace Engine;

    CombatAttackID ResolveNextCombo(CombatAttackID current, CombatInputIntent intent)
    {
        if (intent != CombatInputIntent::COMBO)
        {
            return CombatAttackID::NONE;
        }

        switch (current)
        {
        case Engine::CombatAttackID::NONE:      return CombatAttackID::COMBO_1;
        case Engine::CombatAttackID::COMBO_1:   return CombatAttackID::COMBO_2;
        case Engine::CombatAttackID::COMBO_2:   return CombatAttackID::COMBO_1;
        default:                                return CombatAttackID::COMBO_1;
        }
    }

    // 技の共通発動手続き（初段・次段共通）
    void TriggerAttack(
        CombatAttackID newAttackId,
        CharacterCombatStateComponent& combatState,
        CombatRequestComponent& combatReq,
        CharacterActionResultComponent& actionResult)
    {
        combatState.activeAttackId = newAttackId;
        combatState.attackTimer = 0.0f;
        combatState.hasQueuedCombo = false;
        combatState.isHitRegistered = false;
        combatReq.isExecuted = true; // 要求を消費
        // 技に応じたアニメーションとブレンド時間の設定
        if (newAttackId == CombatAttackID::COMBO_1)
        {
            actionResult.targetAnim = AnimStateType::COMBO1;
            actionResult.currentFadeDuration = 0.1f;
        }
        if (newAttackId == CombatAttackID::COMBO_2)
        {
            actionResult.targetAnim = AnimStateType::COMBO2;
            actionResult.currentFadeDuration = 0.15f;
        }
    }

    void UpdateCurrentAttackTimeline(
        float dt,
        CharacterCombatStateComponent& combatState,
        AnimEventComponent& animEvent,
        CombatRequestComponent& combatReq,
        AnimSequenceAsset* pSeq)
    {
        combatState.attackTimer += dt;

        // 技を実行していない（初段開始前）なら時間更新は不要
        if (combatState.activeAttackId == CombatAttackID::NONE || !pSeq)
        {
            return;
        }

        // 先行入力の受付判定（COMBO_QUEUE 区間内か）
        if (!combatReq.isExecuted && combatReq.intent == CombatInputIntent::COMBO)
        {
            if (IsEventWindowActive(animEvent, pSeq->animState, AnimEventWindowType::COMBO_QUEUE))
            {
                combatState.hasQueuedCombo = true; // 予約
            }
            combatReq.isExecuted = true; // 要求は消費
        }
    }

    bool EvaluateAttackTransition(
        CharacterCombatStateComponent& combatState,
        CombatRequestComponent& combatReq,
        CharacterActionResultComponent& actionResult,
        const AnimSequenceAsset* pSeq)
    {
        // パターンA: 初段の開始（NONE状態で攻撃要求がある場合）
        if (combatState.activeAttackId == CombatAttackID::NONE)
        {
            if (!combatReq.isExecuted && combatReq.intent == CombatInputIntent::COMBO)
            {
                CombatAttackID firstAttack = ResolveNextCombo(CombatAttackID::NONE, combatReq.intent);
                TriggerAttack(firstAttack, combatState, combatReq, actionResult);
            }
            return true;
        }
        // パターンB: 次段へのコンボ遷移（先行予約があり、かつキャンセル可能点に達した場合）
        float cancelPoint = 0.55f; // 必要に応じて pSeq から取得
        if (combatState.hasQueuedCombo && combatState.attackTimer >= cancelPoint)
        {
            CombatAttackID nextAttack = ResolveNextCombo(combatState.activeAttackId, CombatInputIntent::COMBO);
            TriggerAttack(nextAttack, combatState, combatReq, actionResult);
            return true;
        }

        // パターンC: 技の完走・終了（予約なしでモーション時間が満了した場合）
        float duration = (pSeq && pSeq->clip) ? pSeq->clip->GetDurationInSeconds() : 0.0f;
        if (combatState.attackTimer >= duration)
        {
            combatState.activeAttackId = CombatAttackID::NONE;
            combatState.hasQueuedCombo = false;
        }

        return false;
    }
}

namespace Engine
{
    void AttackingStateSystem::Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<
            PlayerTag,
            AnimatorComponent,
            CharacterActionStateComponent,
            CharacterCombatStateComponent,
            AnimEventComponent,
            CombatRequestComponent,
            CharacterActionResultComponent,
            MotionWarpComponent
        >();

        for (auto&& [entity, animator, charaActionState, combatState, animEvent, combatReq, actionResult, motionWarp] : view.each())
        {
            // 戦闘以外では何もしない
            if (charaActionState.currentState != ActionState::ATTACK)
            {
                continue;
            }

            // 現在のアニメーションシーケンスを取得
            AnimSequenceAsset* pAnimSeq = animator.GetCurrentAnimSeq();

            // 時間進行とイベント評価
            UpdateCurrentAttackTimeline(dt, combatState, animEvent, combatReq, pAnimSeq);

            // コンボの遷移判定
            bool isExecuted = EvaluateAttackTransition(combatState, combatReq, actionResult, pAnimSeq);

            // コンボ遷移開始したとき
            // 敵の位置まで移動するための情報を取得
            if (isExecuted && combatReq.targetEntity != entt::null)
            {
                TransformComponent playerTransform = registry.get<TransformComponent>(entity);
                TransformComponent targetTransform = registry.get<TransformComponent>(combatReq.targetEntity);

                DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&playerTransform.position);
                DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&targetTransform.position);
                targetPos = DirectX::XMVectorAdd(
                    targetPos,
                    DirectX::XMVectorScale(DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(playerPos, targetPos)), 1)
                );

                motionWarp.startPosition = playerTransform.position;
                DirectX::XMStoreFloat3(&motionWarp.endPosition, targetPos);
                motionWarp.duration = 0.5f;
                motionWarp.elapsedTime = 0.0f;
                motionWarp.isActive = true;

                motionWarp.targetYaw = Math::CalcLookAtYaw(playerTransform.position, targetTransform.position);
            }

            // 敵の位置まで移動してる時の処理
            // 設定された秒数の間移動処理を行う
            DirectX::XMFLOAT3 vec = {0.0f, 0.0f, 0.0f};
            if (motionWarp.isActive)
            {
                TransformComponent playerTransform = registry.get<TransformComponent>(entity);
                DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&playerTransform.position);
                DirectX::XMVECTOR startPos = DirectX::XMLoadFloat3(&motionWarp.startPosition);
                DirectX::XMVECTOR endPos = DirectX::XMLoadFloat3(&motionWarp.endPosition);

                motionWarp.elapsedTime += dt;
                float t = std::clamp(motionWarp.elapsedTime / motionWarp.duration, 0.0f, 1.0f);
                DirectX::XMVECTOR currentPos = DirectX::XMVectorLerp(startPos, endPos, t);

                DirectX::XMVECTOR targetVec = DirectX::XMVectorSubtract(currentPos, playerPos);
                targetVec = DirectX::XMVectorScale(targetVec, 1.0f / dt);
                if (t >= 1.0f)
                {
                    motionWarp.isActive = false;
                }
                else
                {
                    DirectX::XMStoreFloat3(&vec, targetVec);
                }
            }

            // 移動・旋回の拘束出力
            actionResult.velocity = vec;
            actionResult.targetYaw = motionWarp.targetYaw;
            actionResult.shouldRotate = motionWarp.isActive;
        }
    }
}
