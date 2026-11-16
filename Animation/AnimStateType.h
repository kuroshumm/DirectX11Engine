#pragma once
#include <cstdint>

namespace Engine
{
    enum class AnimStateType : uint8_t
    {
        NONE,
        IDLE,
        WALK,
        WALK_RIGHT,
        WALK_LEFT,
        WALK_BACK,
        RUN,
        COMBO1,
        COMBO2,
        DAMAGE,
        DEATH
    };
}
