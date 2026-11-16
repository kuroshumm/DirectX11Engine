#pragma once
#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
    class CollisionSpawnSystem
    {
    public:
        void Update(entt::registry& registry);
    };
}
