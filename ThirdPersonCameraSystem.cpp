#include "ThirdPersonCameraSystem.h"
#include "Components/TransformComponents.h"
#include "Components/CameraComponents.h"

namespace
{
	using namespace Engine;

	DirectX::XMVECTOR CalcFocusPos(
		const TransformComponent& targetTransform,
		const ThirdPersonCameraConfig& tpsConfig
	)
	{
		DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&targetTransform.position);
		return DirectX::XMVectorAdd(targetPos, DirectX::XMVectorSet(0.0f, tpsConfig.heightOffset, 0.0f, 0.0f));
	}

	DirectX::XMVECTOR CalcCameraPos(
		const DirectX::XMVECTOR focusPos,
		const ThirdPersonCameraComponent& tpsComp
	)
	{
		// カメラの座標計算
		float targetDistance = tpsComp.distance;
		float x = targetDistance * std::cos(tpsComp.pitch) * std::sin(tpsComp.yaw);
		float y = targetDistance * std::sin(tpsComp.pitch);
		float z = targetDistance * std::cos(tpsComp.pitch) * std::cos(tpsComp.yaw);
		return DirectX::XMVectorAdd(focusPos, DirectX::XMVectorSet(x, y, z, 0.0f));
	}

	DirectX::XMVECTOR CalcCameraRotQuat(
		const DirectX::XMVECTOR focusPos,
		const DirectX::XMVECTOR cameraPos
	)
	{
		// 視線方向
		DirectX::XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(focusPos, cameraPos));
		
		// 右方向
		DirectX::XMVECTOR worldUp = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		DirectX::XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, forward));

		// 上方向
		DirectX::XMVECTOR up = DirectX::XMVector3Cross(forward, right);

		// ワールド回転行列を直接構築（行ベクトルに各基底をセット）
		DirectX::XMMATRIX rotMatrix = DirectX::XMMatrixIdentity();
		rotMatrix.r[0] = right;
		rotMatrix.r[1] = up;
		rotMatrix.r[2] = forward;

		// 5. 回転行列から直接クォータニオンへ変換（逆行列・Decompose不要）
		return DirectX::XMQuaternionRotationMatrix(rotMatrix);
	}
}

namespace Engine
{
	void ThirdPersonCameraSystem::Update(
		entt::registry& registry,
		float dt
	)
	{
		auto view = registry.view<
			CameraComponent,
			TransformComponent,
			ThirdPersonCameraComponent,
			ThirdPersonCameraConfig,
			ActiveCameraTag>();

		for (auto [entity, CameraComp, transformComp, tpsComp, tpsConfig] : view.each())
		{
			// ターゲットのEntityを取得
			auto targetEntity = tpsComp.targetEntity;
			if (!registry.valid(targetEntity))
			{
				continue;
			}

			InputSystem& inputSystem = InputSystem::GetInstance();

			if (inputSystem.IsKeyPressed(VK_MBUTTON))
			{
				tpsComp.pitch = tpsConfig.pitch;
				tpsComp.yaw = 0.0f;
				tpsComp.distance = tpsConfig.distance;
			}

			// マウス入力によるピッチ・ヨーの更新
			Vector2 mouseDelta;
			if (inputSystem.TryGetMouseDelta(mouseDelta))
			{
				// pitchの計算
				float pitch = tpsComp.pitch + mouseDelta.y * tpsConfig.rotateSensitivity;
				tpsComp.pitch = std::clamp(pitch, tpsConfig.minPitch, tpsConfig.maxPitch);
				// yawの計算
				tpsComp.yaw = tpsComp.yaw + mouseDelta.x * tpsConfig.rotateSensitivity;
			}

			// ターゲットのTransform取得
			const auto& targetTransform = registry.get<TransformComponent>(targetEntity);

			// 注視点（ターゲット位置 + 高さオフセット）の計算
			DirectX::XMVECTOR focusPos = CalcFocusPos(targetTransform, tpsConfig);

			// カメラの座標を計算
			DirectX::XMVECTOR cameraPos = CalcCameraPos(focusPos, tpsComp);
			DirectX::XMStoreFloat3(&transformComp.position, cameraPos);

			// カメラのワールド回転クォータニオンを計算
			DirectX::XMVECTOR cameraRotQuat = CalcCameraRotQuat(focusPos, cameraPos);
			DirectX::XMStoreFloat4(&transformComp.rotation, cameraRotQuat);
		}
	}
}