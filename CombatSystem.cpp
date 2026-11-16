#include "CombatSystem.h"
#include "InputSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"
#include <Components/TransformComponents.h>

namespace
{
    using namespace Engine;

    /// <summary>
    /// 攻撃の入力から次に再生するアクションを計算
    /// </summary>
    /// <param name="combatInputIntent"></param>
    /// <returns></returns>
    CombatAttackID CalcNextCombatAttackID(
        CombatInputIntent& combatInputIntent,
        CombatAttackID activeAttackID
    )
    {
        // コンボ以外は受け付けない
        if (combatInputIntent != CombatInputIntent::COMBO)
        {
            return CombatAttackID::NONE;
        }
    }

    std::vector<entt::entity> CalcGetEnemyList(entt::registry& registry)
    {
        std::vector<entt::entity> enemyList;

        auto enemyView = registry.view<EnemyTag, CharacterActionStateComponent>();
        for (auto&& [entity, actionState] : enemyView.each())
        {
            if (actionState.currentState == ActionState::DEAD)
            {
                continue;
            }
            enemyList.push_back(entity);
        }

        return enemyList;
    }

    float CalcDistance(
        const entt::registry& registry,
        entt::entity startEntity,
        entt::entity endEntity)
    {
        const DirectX::XMFLOAT3& startPos = registry.get<TransformComponent>(startEntity).position;
        const DirectX::XMFLOAT3& endPos = registry.get<TransformComponent>(endEntity).position;

        DirectX::XMVECTOR startVec = DirectX::XMLoadFloat3(&startPos);
        DirectX::XMVECTOR endVec = DirectX::XMLoadFloat3(&endPos);
        DirectX::XMVECTOR vec = DirectX::XMVectorSubtract(endVec, startVec);

        float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(vec));

        return distance;
    }

    entt::entity CalcTargetEnemy(
        const entt::registry& registry,
        const DirectX::XMFLOAT3& inputDir,
        const entt::entity player,
        const std::vector<entt::entity>& enemyList
    )
    {
        entt::entity targetEnemy = entt::null;
        if (enemyList.empty())
        {
            return targetEnemy;
        }

        // プレイヤー座標の取得
        const auto& playerTransform = registry.get<TransformComponent>(player);
        const DirectX::XMFLOAT3& playerPos = playerTransform.position;

        // 基準方向
        float dirX = inputDir.x;
        float dirZ = inputDir.z;

        if (dirX * dirX + dirZ * dirZ <= 0.0001f)
        {
            // 無入力時: プレイヤーの現在の正面（-Z）をクォータニオンから取得
            DirectX::XMVECTOR playerQuatVec = DirectX::XMLoadFloat4(&playerTransform.rotation);
            DirectX::XMVECTOR localForward = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
            DirectX::XMVECTOR forwardVec = DirectX::XMVector3Rotate(localForward, playerQuatVec);

            DirectX::XMFLOAT3 forward;
            DirectX::XMStoreFloat3(&forward, forwardVec);
            dirX = forward.x;
            dirZ = forward.z;
        }

        // ターゲット対象となる角度
        float targetRangeRad = DirectX::XMConvertToRadians(45.0f);
        float maxDot = std::cos(targetRangeRad);

        for (entt::entity currentEnemy : enemyList)
        {
            const auto& enemyPos = registry.get<TransformComponent>(currentEnemy).position;

            float dx = enemyPos.x - playerPos.x;
            float dz = enemyPos.z - playerPos.z;
            float distSq = dx * dx + dz * dz;
            if (distSq < 0.0001f) continue; // 密着時はスキップ

            // 逆数乗算（重い割り算を1回に抑え、乗算に変換する最適化）
            float invDist = 1.0f / std::sqrt(distSq);
            float enemyDirX = dx * invDist;
            float enemyDirZ = dz * invDist;

            // 水平内積 (cos θ)
            float dot = dirX * enemyDirX + dirZ * enemyDirZ;
            // 最も入力正面に近い敵を更新
            if (dot > maxDot)
            {
                maxDot = dot;
                targetEnemy = currentEnemy;
            }
        }
        return targetEnemy;
    }
}

namespace Engine
{
    void CombatSystem::Update(entt::registry& registry)
    {
        // 敵のリストを取得
        std::vector<entt::entity> enemyList = CalcGetEnemyList(registry);

        auto view = registry.view<
            PlayerTag,
            PlayerInputComponent,
            LocomotionProposalComponent,
            CharacterCombatStateComponent,
            CombatRequestComponent
        >();

        for (auto&& [entity, playerInput, locomotionProposal, combatState, combatReq] : view.each())
        {
            // アクションの設定
            combatReq.intent = playerInput.bufferAction;
            combatReq.isExecuted = false;

            // 攻撃対象のターゲット計算
            // TODO：周囲の状況や入力から適切なターゲットを計算
            combatReq.targetEntity = CalcTargetEnemy(registry, locomotionProposal.velocity, entity, enemyList);
        }
    }
}
