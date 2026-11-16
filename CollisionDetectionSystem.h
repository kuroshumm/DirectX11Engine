#pragma once
#include <entt/entt.hpp>

namespace Engine
{
    class CollisionDetectionSystem
    {
    public:
        void Update(entt::registry& registry);
    };
}
