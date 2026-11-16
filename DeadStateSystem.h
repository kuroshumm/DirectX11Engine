#pragma once
#include <entt/entt.hpp>

namespace Engine
{
    class DeadStateSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
