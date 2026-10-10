#pragma once
#include <entt/entt.hpp>

#include "AnimatorSystem.h"

namespace Engine
{
	class ActionApplierSystem
	{
	public:
		void Update(entt::registry& registry, AnimatorSystem& animatorSystem, float dt);
	};
}