#pragma once

#include <entt/entt.hpp>

#include "InputSystem.h"

namespace Engine
{
    class ThirdPersonCameraSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
