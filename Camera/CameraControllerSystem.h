#pragma once
#include <entt/entt.hpp>

namespace Engine
{
    class CameraControllerSystem
    {
    public:
        void Update(entt::registry& registry, float dt);
    };
}
