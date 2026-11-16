#pragma once
#include <entt/entt.hpp>
#include <DirectXMath.h>
#include "Components.h"

namespace Engine
{
    class HierarchyTransformSystem
    {
    public:
        void Update(entt::registry& registry);

    private:
        void UpdateSubtree(
            entt::registry& registry,
            entt::entity current,
            const DirectX::XMMATRIX& parentWorldMatrix,
            bool parentIsDirty);

        DirectX::XMMATRIX ComputeLocalMatrix(const TransformComponent& transform);
    };
}
