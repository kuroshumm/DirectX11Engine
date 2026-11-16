#pragma once

#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
    class AnimEventSystem
    {
    public:
        void Update(entt::registry& registry);
    };
}
