#pragma once

#include <entt/entt.hpp>

namespace Engine
{
    class CharacterActionStateSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
