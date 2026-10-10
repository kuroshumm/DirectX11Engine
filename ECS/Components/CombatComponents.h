#pragma once
#include <cstdint>
#include <string>
#include <DirectXMath.h>
#include <entt/entt.hpp>

namespace Engine
{
    enum class CombatInputIntent : uint8_t
    {
        NONE,
        COMBO,
        STRIKE,
        COUNTER,
        GUARD,
    };

    enum class CombatAttackID : uint8_t
    {
        NONE,
        COMBO_1,
        COMBO_2,
        CLOSE_STRIKE,
        LEAP_STRIKE,
        COUNTER_ATTACK,
        GUARD,
    };

    enum class ActionState : uint8_t
    {
        LOCOMOTION,
        ATTACK,
        DODGE,
        HIT_STUN
    };

    struct CombatRequestComponent
    {
        CombatAttackID combatAttackID = CombatAttackID::NONE;
        entt::entity targetEntity = entt::null;
        bool isExecuted = false;
    };

    struct CharacterActionStateComponent
    {
        ActionState currentState = ActionState::LOCOMOTION;
        float stateTimer = 0.0f;
    };

    struct CharacterCombatStateComponent
    {
        CombatAttackID activeAttackId = CombatAttackID::NONE;
        float attackTimer = 0.0f;
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

    struct ComboTransitionRule
    {

    };
}
