#pragma once

#include <entt/entt.hpp>

namespace Engine
{
	class AttackingStateSystem
	{
	public:
		void Update(entt::registry& registry, float dt);
	};
}