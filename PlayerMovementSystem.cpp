#include <DirectXMath.h>

#include "PlayerMovementSystem.h"
#include "Components/TransformComponents.h"
#include "Components/CameraComponents.h"
#include "Components/CharacterComponents.h"
#include "MathUtils.h"

namespace
{
    using namespace Engine;

    LocomotionState CalcCurrentLocomotionState(bool hasMoveInput, bool isSprinting)
    {
        if (!hasMoveInput)
        {
            return LocomotionState::IDLE;
        }

        LocomotionState state = isSprinting
            ? LocomotionState::RUN
            : LocomotionState::WALK;

        return state;
    }

    struct LocomotionDirResult
    {
        float targetYaw = 0.0f;
        MoveDirection dir = MoveDirection::NONE;
    };

    // 進行方向を向くタイプの移動
    LocomotionDirResult CalcOrientationLocomotion(
        const DirectX::XMFLOAT3& moveDir
    )
    {
        LocomotionDirResult result;

        result.targetYaw = Math::CalcYawFromDirection(moveDir);
        result.dir = MoveDirection::FORWARD;

        return result;
    }

    // カニ歩き方式
    LocomotionDirResult CalcStrafeLocomotion(
        const DirectX::XMFLOAT2& moveAxis,
        const DirectX::XMFLOAT3& camForward
    )
    {
        LocomotionDirResult result{};

        if (moveAxis.y > 0.1f)       result.dir = MoveDirection::FORWARD;
        else if (moveAxis.y < -0.1f) result.dir = MoveDirection::BACK;
        else if (moveAxis.x < -0.1f) result.dir = MoveDirection::LEFT;
        else if (moveAxis.x > 0.1f)  result.dir = MoveDirection::RIGHT;

        result.targetYaw = Math::CalcYawFromDirection(camForward);

        return result;
    }

    // 
    LocomotionDirResult CalcLocomotionDirection(
        MovementMode mode,
        const DirectX::XMFLOAT2& moveAxis,
        const DirectX::XMFLOAT3& moveDir,
        const DirectX::XMFLOAT3& camForward)
    {
        switch (mode)
        {
        case MovementMode::ORIENTATION:
            return CalcOrientationLocomotion(moveDir);
        case MovementMode::STRAFE:
            return CalcStrafeLocomotion(moveAxis, camForward);
        default:
            return CalcOrientationLocomotion(moveDir);
        }
    }

}

namespace Engine
{
    void PlayerMovementSystem::Update(entt::registry& registry, float dt)
    {
        // アクティブなカメラを取得
        auto cameraView = registry.view<TransformComponent, ActiveCameraTag>();
        auto activeCameraEntity = cameraView.front();

        auto view = registry.view<
            PlayerTag,
            PlayerInputComponent,
            LocomotionProposalComponent,
            CharacterMovementConfigComponent
        >();
        for (auto [entity, playerInput, locomotionProposal, moveConfig] : view.each())
        {
            bool hasMoveInput = playerInput.moveAxis.x != 0.0f || playerInput.moveAxis.y != 0.0f;
            bool isSprinting = playerInput.isSprintPressed;

            locomotionProposal.state = CalcCurrentLocomotionState(hasMoveInput, isSprinting);

            // 移動先の方向を向く
            DirectX::XMFLOAT3 camForwardFloat = { 0.0f, 0.0f, 0.0f };
            DirectX::XMFLOAT3 moveDir = {0.0f, 0.0f, 0.0f};
            if (hasMoveInput)
            {
                // カメラのクォータニオン取得
                auto cameraTransform = registry.get<TransformComponent>(activeCameraEntity);
                DirectX::XMVECTOR cameraQuat = DirectX::XMLoadFloat4(&cameraTransform.rotation);

                // 前方、右方ベクトルの取得
                DirectX::XMVECTOR cameraFoward3d = DirectX::XMVector3Rotate(DirectX::XMVectorSet(0.0f,0.0f,1.0f,0.0f), cameraQuat);
                DirectX::XMVECTOR cameraRight3d = DirectX::XMVector3Rotate(DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), cameraQuat);
                // 前方、右方ベクトルの水平射影
                DirectX::XMVECTOR cameraFoward = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(cameraFoward3d, 0.0f));
                DirectX::XMVECTOR cameraRight = DirectX::XMVector3Normalize(DirectX::XMVectorSetY(cameraRight3d, 0.0f));

                // キー入力された向きをカメラの向きを基準に変換
                DirectX::XMVECTOR moveAxisCamera = DirectX::XMVectorAdd(
                    DirectX::XMVectorScale(cameraFoward, playerInput.moveAxis.y),
                    DirectX::XMVectorScale(cameraRight, playerInput.moveAxis.x)
                );
                moveAxisCamera = DirectX::XMVector3Normalize(moveAxisCamera);

                // 目標yawの算出
                DirectX::XMStoreFloat3(&moveDir, moveAxisCamera);
                
                LocomotionDirResult dirResult = CalcLocomotionDirection(
                    moveConfig.mode,
                    playerInput.moveAxis,
                    moveDir,
                    camForwardFloat
                );
                locomotionProposal.yaw = dirResult.targetYaw;
                locomotionProposal.dir = dirResult.dir;
            }

            // 移動ベクトル
            locomotionProposal.velocity = moveDir;

            
        }
    }
}
