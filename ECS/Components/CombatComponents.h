#pragma once
#include <DirectXMath.h>
#include <entt/entt.hpp>
#include "CombatType.h"

namespace Engine
{
    struct CombatRequestComponent
    {
        CombatInputIntent intent = CombatInputIntent::NONE;
        entt::entity targetEntity = entt::null;
        bool isExecuted = false;
    };

    struct CharacterActionStateComponent
    {
        ActionState currentState = ActionState::LOCOMOTION;
        float stateTimer = 0.0f;
        bool isChangeState = false;
    };

    struct CharacterCombatStateComponent
    {
        CombatAttackID activeAttackId = CombatAttackID::NONE;
        float attackTimer = 0.0f;
        bool hasQueuedCombo = false;
        bool isHitRegistered = false;
    };

    struct CharacterActionResultComponent
    {
        DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
        float targetYaw = 0.0f;
        bool shouldRotate = false;
        float turnSpeed = 10.0f;

        AnimStateType targetAnim = AnimStateType::NONE;
        float currentFadeDuration = 0.25f;
    };

    struct MotionWarpComponent
    {
        DirectX::XMFLOAT3 startPosition;
        DirectX::XMFLOAT3 endPosition;
        float targetYaw;
        float duration;
        float elapsedTime;
        bool isActive;
    };

    struct CombatAttackPresetComponent
    {
        const CombatAttackPreset* pPreset = &CombatPresets::HumanoidDefault;
    };

    struct DamageRequestComponent
    {
        bool hasPendingDamage = false;
        entt::entity attackerEntity = entt::null;
        int damage = 0;
    };

    enum class DamageResultType : uint8_t
    {
        NONE,

        DAMAGED,
        DEAD,

        COUNT
    };

    struct DamageResultComponent
    {
        DamageResultType result = DamageResultType::NONE;
        entt::entity attackerEntity = entt::null;
    };

    struct DamageStateComponent
    {
        bool isActive = false;
        float damageTimer = 0.0f;
    };

    struct DeadStateComponent
    {
        bool isActive = false;
        float elapsedTimer = 0.0f;
        float destroyTimer = 2.0f;
    };

    struct HealthComponent
    {
        int maxHp = 3;
        int hp = 3;
    };
}
