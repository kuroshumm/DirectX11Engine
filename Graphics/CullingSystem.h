#pragma once
#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
	class CullingSystem
	{
	public:
		void Update(entt::registry& registry);
	};
}