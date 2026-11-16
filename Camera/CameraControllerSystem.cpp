#include "CameraControllerSystem.h"
#include "Components.h"
#include "InputSystem.h"
#include "Vector2.h"
#include "Vector3.h"
#include <algorithm>

namespace Engine
{
    void CameraControllerSystem::Update(entt::registry& registry, float dt)
    {
        InputSystem& input = InputSystem::GetInstance();
        auto view = registry.view<TransformComponent, FreeLookCameraControllerComponent>();

        for (auto [entity, transform, controller] : view.each())
        {
            // 1. 回転処理 (マウス中ボタンドラッグ等)
            if (input.IsKeyPressed(VK_MBUTTON))
            {
                Vector2 mouseDelta;
                if (input.TryGetMouseDelta(mouseDelta))
                {
                    controller.yaw += mouseDelta.x * controller.sensitivity * dt;
                    controller.pitch += mouseDelta.y * controller.sensitivity * dt;

                    // ピッチ角の制限（上下反転防止）
                    controller.pitch = std::clamp(controller.pitch, controller.minPitch, controller.maxPitch);

                    // オイラー角からクォータニオンを生成して Transform に格納
                    DirectX::XMVECTOR qRot = DirectX::XMQuaternionRotationRollPitchYaw(controller.pitch, controller.yaw, 0.0f);
                    DirectX::XMStoreFloat4(&transform.rotation, qRot);
                    transform.isDirty = true;
                }
            }

            // 2. 移動処理 (WASD 入力)
            DirectX::XMFLOAT3 moveDirLocal{ 0.0f, 0.0f, 0.0f };
            if (input.IsKeyPressed('W')) moveDirLocal.z += 1.0f;
            if (input.IsKeyPressed('S')) moveDirLocal.z -= 1.0f;
            if (input.IsKeyPressed('A')) moveDirLocal.x -= 1.0f;
            if (input.IsKeyPressed('D')) moveDirLocal.x += 1.0f;

            float lengthSq = moveDirLocal.x * moveDirLocal.x + moveDirLocal.y * moveDirLocal.y + moveDirLocal.z * moveDirLocal.z;
            if (lengthSq > 0.0001f)
            {
                // カメラの現在の姿勢（回転クォータニオン）からローカル移動ベクトルをワールド方向へ変換
                DirectX::XMVECTOR vLocalMove = DirectX::XMLoadFloat3(&moveDirLocal);
                DirectX::XMVECTOR vRot = DirectX::XMLoadFloat4(&transform.rotation);
                DirectX::XMVECTOR vWorldMove = DirectX::XMVector3Rotate(DirectX::XMVector3Normalize(vLocalMove), vRot);

                DirectX::XMFLOAT3 worldDir;
                DirectX::XMStoreFloat3(&worldDir, vWorldMove);

                transform.position.x += worldDir.x * controller.moveSpeed * dt;
                transform.position.y += worldDir.y * controller.moveSpeed * dt;
                transform.position.z += worldDir.z * controller.moveSpeed * dt;
                transform.isDirty = true;
            }
        }
    }
}
