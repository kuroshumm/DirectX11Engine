#pragma once
#include <DirectXMath.h>
#include <entt/entt.hpp>
#include "Frustum.h"

namespace Engine
{
    // Camera projection and view parameters
    struct CameraComponent
    {
        float fov = DirectX::XM_PIDIV4; // 45 deg
        float aspectRatio = 16.0f / 9.0f;
        float nearZ = 0.1f;
        float farZ = 1000.0f;
        
        DirectX::XMFLOAT4X4 viewMatrix;
        DirectX::XMFLOAT4X4 projectionMatrix;
        Frustum frustum;
        bool isProjectionDirty = true;
    };

    // Free-look camera controller
    struct FreeLookCameraControllerComponent
    {
        float moveSpeed = 50.0f;
        float sensitivity = 0.3f;
        float pitch = 0.0f;
        float yaw = 0.0f;
        float minPitch = -DirectX::XM_PIDIV2 + 0.01f;
        float maxPitch = DirectX::XM_PIDIV2 - 0.01f;
    };

    struct ThirdPersonCameraConfig
    {
        float distance = 3.5f;
        float heightOffset = 1.4f;
        
        float pitch = 0.18f;
        float minPitch = -0.5f;
        float maxPitch = 1.3f;

        float rotateSensitivity = 0.005f;
        int t = 0;
    };

    // Third person camera component
    struct ThirdPersonCameraComponent
    {
        entt::entity targetEntity = entt::null;

        float distance = 3.5f;
        float heightOffset = 1.4f;
        float pitch = 0.18f;
        float yaw = 0.0f;
    };

    struct ActiveCameraTag {};
}
