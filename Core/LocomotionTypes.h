#pragma once
#include <cstdint>
#include <array>
#include "AnimStateType.h"

namespace Engine
{
    enum class MoveDirection : uint8_t
    {
        NONE,

        FORWARD,
		RIGHT,
		LEFT,
		BACK,

        COUNT
    };

    struct LocomotionAnimPreset
    {
        std::array<AnimStateType, static_cast<size_t>(MoveDirection::COUNT)> clips;
    };

    namespace LocomotionPresets
    {
        static constexpr LocomotionAnimPreset HumanoidNormal = { {
				AnimStateType::IDLE,
				AnimStateType::WALK,
				AnimStateType::WALK_RIGHT,
				AnimStateType::WALK_LEFT,
				AnimStateType::WALK_BACK,
        } };
    }
}
