#include "SceneCameraOperator.h"
#include "InputSystem.h"
#include <algorithm>
#include "Vector3.h"

namespace Engine
{
    SceneCameraOperator::SceneCameraOperator(std::weak_ptr<Camera> pCamera)
    {
        m_pCamera = pCamera;
    }

    void SceneCameraOperator::Update(float deltaTime)
    {
        InputSystem& inputSystem = InputSystem::GetInstance();
        
        const std::shared_ptr<Camera>& pCamera = m_pCamera.lock();
        if (pCamera == nullptr)
        {
            return;
        }
        
        // 回転
        Rotate(deltaTime, pCamera);

        // 移動
        Move(deltaTime, pCamera);
    }

    void SceneCameraOperator::Rotate(float deltaTime, const std::shared_ptr<Camera>& pCamera)
    {
        InputSystem& inputSystem = InputSystem::GetInstance();

        if (!inputSystem.IsKeyPressed(VK_MBUTTON))
        {
            return;
        }

        // マウスの移動量を取得
        Vector2 mouseDelta;
        if (!inputSystem.TryGetMouseDelta(mouseDelta))
        {
            return;
        }

        DirectX::XMFLOAT3 currentRot = pCamera->GetTransform().GetRotation();

        // ピクセル変位をラジアンにスケールダウンしてから加算する
        currentRot.x += mouseDelta.y * SENSITIVITY * deltaTime;
        currentRot.y += mouseDelta.x * SENSITIVITY * deltaTime;

        // スケールダウンされた安全な状態値に対して、限界値（±88.8°）の天井を適用する
        //currentRot.x = std::clamp(currentRot.x, -1.55f, 1.55f);
        currentRot.z = 0.0f; // Roll成分の永久拘束

        // カメラを回転
        pCamera->GetTransform().SetRotation(currentRot);
    }

    void SceneCameraOperator::Move(float deltaTime, const std::shared_ptr<Camera>& pCamera)
    {
        InputSystem& inputSystem = InputSystem::GetInstance();

        // 入力ベクトルの算出
        Vector3 moveDir = Vector3::zero;
        if (inputSystem.IsKeyPressed('W')) moveDir += Vector3::forward; // 前進
        if (inputSystem.IsKeyPressed('S')) moveDir -= Vector3::forward; // 後退
        if (inputSystem.IsKeyPressed('A')) moveDir -= Vector3::right;   // 左移動
        if (inputSystem.IsKeyPressed('D')) moveDir += Vector3::right;   // 右移動

        // カメラのワールド行列を取得
        DirectX::XMMATRIX cameraWorldMatrix = pCamera->GetTransform().GetWorldMatrix();
        
        // ローカルベクトルをSIMD型へLoadし、カメラの回転を適用（乗算変換）
        DirectX::XMVECTOR vMoveDir = DirectX::XMVectorSet(moveDir.x, moveDir.y, moveDir.z, 0.0f);
        DirectX::XMVECTOR vWorldDir = DirectX::XMVector3TransformNormal(vMoveDir, cameraWorldMatrix);

        // ベクトルの正規化（斜め移動時の速度の肥大化を防ぐため単位ベクトル化）
        vWorldDir = DirectX::XMVector3Normalize(vWorldDir);

        // SIMD型から標準のfloat型へStoreし位置を加算更新
        DirectX::XMFLOAT3 dirInfo;
        DirectX::XMStoreFloat3(&dirInfo, vWorldDir);

        DirectX::XMFLOAT3 currentPos = pCamera->GetTransform().GetPosition();
        currentPos.x += dirInfo.x * moveSpped * deltaTime;
        currentPos.y += dirInfo.y * moveSpped * deltaTime;
        currentPos.z += dirInfo.z * moveSpped * deltaTime;

        pCamera->GetTransform().SetPosition(currentPos);
    }
}
