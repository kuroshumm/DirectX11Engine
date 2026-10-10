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

    struct CharacterMovementConfigComponent
    {
        float walkSpeed = 0.5f;
        float springSpeed = 6.5f;
        float turnSpeed = 10.0f;
    };

    enum class LocomotionState { IDLE, WALK, RUN };

    struct CharacterLocomotionComponent
    {
        LocomotionState currentState = LocomotionState::IDLE;
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
}
