#include <DirectXMath.h>

#include "PlayerMovementSystem.h"
#include "Components/TransformComponents.h"
#include "Components/CameraComponents.h"
#include "Components/CharacterComponents.h"

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
			LocomotionProposalComponent
		>();
		for (auto [entity, playerInput, locomotionProposal] : view.each())
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
				//locomotionProposal.yaw = std::atan2(moveDir.x, moveDir.z);

				// 移動方向
				MoveDirection dir = MoveDirection::NONE;
				if (playerInput.moveAxis.y > 0.1f) dir = MoveDirection::FORWARD;  // W
				else if (playerInput.moveAxis.y < -0.1f) dir = MoveDirection::BACK; // S
				else if (playerInput.moveAxis.x < -0.1f) dir = MoveDirection::LEFT; // A
				else if (playerInput.moveAxis.x > 0.1f) dir = MoveDirection::RIGHT; // D
				locomotionProposal.dir = dir;

				// 2. 目標Yaw角の決定（ストレイフ移動: カメラ正面を向き続ける）
				// カメラの前方水平ベクトルからYaw角を算出
				DirectX::XMStoreFloat3(&camForwardFloat, cameraFoward);
				locomotionProposal.yaw = std::atan2(-camForwardFloat.x, -camForwardFloat.z);


				//// 目標yawの算出
				//DirectX::XMFLOAT3 dir;
				//DirectX::XMStoreFloat3(&dir, moveAxisCamera);
				//float targetYaw = std::atan2(dir.x, dir.z);
				//// 目標yawに到達するためのクォータニオンを計算
				//DirectX::XMVECTOR upAxis = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				//DirectX::XMVECTOR targetQuat = DirectX::XMQuaternionRotationNormal(upAxis, targetYaw);

				//// 目標yawまでの回転
				//float t = std::clamp(charaMoveConfig.turnSpeed * dt, 0.0f, 1.0f);
				//DirectX::XMVECTOR currentQuat = DirectX::XMLoadFloat4(&transform.rotation);
				//DirectX::XMVECTOR newQuat = DirectX::XMQuaternionSlerp(currentQuat, targetQuat, t);

				//// 回転を適用
				//DirectX::XMStoreFloat4(&transform.rotation, newQuat);
			}

			// 移動ベクトル
			locomotionProposal.velocity = moveDir;

			
		}
	}
}