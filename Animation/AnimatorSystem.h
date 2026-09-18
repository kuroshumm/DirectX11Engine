#pragma once

#include <entt/entt.hpp>
#include "Components.h"

namespace Engine
{
	class AnimatorSystem
	{
	public:
		void Update(entt::registry& registry, float dt);
		void Play(entt::registry& registry, entt::entity entity, std::string stateName);
		void CrossFade(entt::registry& registry, entt::entity entity, std::string stateName, float transitionTime);
	};
}