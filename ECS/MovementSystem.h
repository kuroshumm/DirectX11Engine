#pragma once

#include <entt/entt.hpp>

namespace Engine
{
    class MovementSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
