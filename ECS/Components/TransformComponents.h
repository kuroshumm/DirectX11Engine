#pragma once
#include <cstdint>
#include <DirectXMath.h>
#include <entt/entt.hpp>

namespace Engine
{
    struct TransformComponent
    {
        DirectX::XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
        DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4X4 worldMatrix;

        bool isDirty = true;
    };

    struct HierarchyComponent
    {
        entt::entity parent = entt::null;
        entt::entity firstChild = entt::null;
        entt::entity nextSibling = entt::null;
        entt::entity prevSibling = entt::null;

        uint32_t depth = 0;
    };
}
