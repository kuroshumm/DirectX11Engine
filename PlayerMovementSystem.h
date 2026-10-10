#pragma once

#include <entt/entt.hpp>

namespace Engine
{
	class AnimatorSystem;

	class PlayerMovementSystem
	{
	public:
		void Update(entt::registry& registry, float dt);
	};
}