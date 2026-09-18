#include "MovementSystem.h"
#include "Components.h"

namespace Engine
{
	void MovementSystem::Update(entt::registry& registry, float dt)
	{
		auto entities = registry.view<TransformComponent, VelocityComponent>();

		for(auto [entity, transform, velocity] : entities.each())
		{
			transform.position.x += velocity.velocity.x * dt;
			transform.position.y += velocity.velocity.y * dt;
			transform.position.z += velocity.velocity.z * dt;
		}
	}
}