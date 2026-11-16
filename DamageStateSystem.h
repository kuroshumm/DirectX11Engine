#pragma once
#include <entt/entt.hpp>

namespace Engine
{
    class DamageStateSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
