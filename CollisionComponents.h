#pragma once
#include "CollisionType.h"
#include "AnimStateType.h"
#include <unordered_set>

namespace Engine
{
    struct AttackCollisionComponent
    {
        entt::entity owner = entt::null;
        AnimStateType sourceState = AnimStateType::NONE;
        DirectX::XMFLOAT3 localOffset = { 0.0f, 1.0f, 0.0f };
        CollisionShapeDesc shapeDesc = SphereShapeDesc{ 0.5f };
        WorldCollisionVolume worldVolume = DirectX::BoundingSphere{};
        int damage = 0;
        std::unordered_set<entt::entity> hitEntities;
    };

    struct DamageableCollisionComponent
    {
        bool isActive = false;
        DirectX::XMFLOAT3 localOffset = { 0.0f, 1.0f, 0.0f };
        CollisionShapeDesc shapeDesc = SphereShapeDesc{ 0.5f };
        WorldCollisionVolume worldVolume = DirectX::BoundingSphere{};
    };

    struct CollisionVisualizeComponent
    {
        entt::entity ownerCollision = entt::null;
    };

    struct CollisionVisualizeTag {};
}
