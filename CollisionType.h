#pragma once
#include <variant>
#include <vector>
#include <DirectXMath.h>
#include <DirectXCollision.h>

namespace Engine
{
    struct SphereShapeDesc
    {
        float radius;
    };

    struct BoxShapeDesc
    {
        DirectX::XMFLOAT3 halfExtents{ 0.5f, 0.5f, 0.5f };
    }; 

    // Sphere設定 か Box設定 のどちらか一方を保持する型
    using CollisionShapeDesc = std::variant<SphereShapeDesc, BoxShapeDesc>;
    // 2. ランタイム交差判定用のワールド形状（DirectX標準の球 または OBB）
    using WorldCollisionVolume = std::variant<DirectX::BoundingSphere, DirectX::BoundingOrientedBox>;
}
