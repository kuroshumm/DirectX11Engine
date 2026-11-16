#pragma once

#include <entt/entt.hpp>
#include "AnimatorSystem.h"

namespace Engine
{
    class PlayerInputSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
