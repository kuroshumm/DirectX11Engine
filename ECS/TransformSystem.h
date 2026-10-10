#pragma once
#include <entt/entt.hpp>

namespace Engine
{
	class TransformSystem
	{
	public:
		void Update(entt::registry& registry);
	};
}