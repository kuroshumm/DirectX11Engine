#pragma once
#include <DirectXMath.h>
#include "LocomotionTypes.h"
#include "CombatComponents.h"

namespace Engine
{
    struct VelocityComponent
    {
        DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
    };

    struct PlayerInputComponent
    {
        DirectX::XMFLOAT2 moveAxis{ 0.0f, 0.0f };
        bool isSprintPressed = false;

        CombatInputIntent bufferAction = CombatInputIntent::NONE;
        float actionBufferTimer = 0.0f;
    };

    enum class MovementMode : uint8_t
    {
        ORIENTATION,
        STRAFE
    };

    struct CharacterMovementConfigComponent
    {
        float walkSpeed = 3.0f;
        float springSpeed = 6.5f;
        float turnSpeed = 10.0f;

        MovementMode mode = MovementMode::ORIENTATION;
    };

    enum class LocomotionState { NONE, IDLE, WALK, RUN };

    struct CharacterLocomotionComponent
    {
        LocomotionState currentState = LocomotionState::NONE;
    };

    struct LocomotionProposalComponent
    {
        DirectX::XMFLOAT3 velocity{ 0.0f, 0.0f, 0.0f };
        MoveDirection dir = MoveDirection::NONE;
        float yaw = 0.0f;
        LocomotionState state = LocomotionState::IDLE;
    };

    struct LocomotionAnimComponent
    {
        const LocomotionAnimPreset* pPreset = &LocomotionPresets::HumanoidNormal;
    };

    struct PlayerTag {};
    struct EnemyTag {};
}
