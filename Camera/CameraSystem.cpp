#include "CameraSystem.h"
#include "Components.h"

namespace Engine
{
    void CameraSystem::Update(entt::registry& registry)
    {
        auto view = registry.view<TransformComponent, CameraComponent>();

        for (auto [entity, transform, camera] : view.each())
        {
            // 1. プロジェクション行列の更新 (FOV/アスペクト比/クリップ面変更時)
            if (camera.isProjectionDirty)
            {
                DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveFovLH(
                    camera.fov,
                    camera.aspectRatio,
                    camera.nearZ,
                    camera.farZ
                );
                DirectX::XMStoreFloat4x4(&camera.projectionMatrix, proj);
                camera.isProjectionDirty = false;
            }

            // 2. ビュー行列の更新 (M_View = M_World^-1)
            DirectX::XMMATRIX worldMat = DirectX::XMLoadFloat4x4(&transform.worldMatrix);
            DirectX::XMMATRIX viewMat = DirectX::XMMatrixInverse(nullptr, worldMat);
            DirectX::XMStoreFloat4x4(&camera.viewMatrix, viewMat);

            // 3. 視錐台 (Frustum) の更新
            DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4(&camera.projectionMatrix);
            camera.frustum.Update(viewMat, projMat);
        }
    }
}
