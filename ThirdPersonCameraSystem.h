#pragma once

#include <entt/entt.hpp>

#include "Components.h"
#include "InputSystem.h"

namespace Engine
{
	class ThirdPersonCameraSystem
	{
	public:
		void Update(entt::registry& registry, float dt);
	};
}