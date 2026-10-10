#include <algorithm>

#include "ActionApplierSystem.h"
#include "Components/TransformComponents.h"
#include "Components/AnimationComponents.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"

namespace Engine
{
	void ActionApplierSystem::Update(entt::registry& registry, AnimatorSystem& animatorSystem, float dt)
	{
		auto view = registry.view<
			PlayerTag,
			CharacterActionResultComponent,
			AnimatorComponent,
			VelocityComponent,
			TransformComponent
		>();

		for (auto&& [entity, charaActionResult, animator, velocity, transform] : view.each())
		{
			// 速度の反映
			velocity.velocity = charaActionResult.velocity;

			// 旋回の反映
			if (charaActionResult.shouldRotate)
			{
				// 旋回先のQuaternionを取得
				DirectX::XMVECTOR upAxis = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				DirectX::XMVECTOR targetQuat = DirectX::XMQuaternionRotationNormal(upAxis, charaActionResult.targetYaw);

				// 旋回
				float t = std::clamp(charaActionResult.turnSpeed * dt, 0.0f, 1.0f);
				DirectX::XMVECTOR currentQuat = DirectX::XMLoadFloat4(&transform.rotation);
				DirectX::XMVECTOR newQuat = DirectX::XMQuaternionSlerp(currentQuat, targetQuat, t);

				// 旋回を反映
				DirectX::XMStoreFloat4(&transform.rotation, newQuat);
			}

			// アニメーションの反映
			if (charaActionResult.targetAnim != AnimStateType::NONE)
			{
				animatorSystem.CrossFade(
					registry,
					entity,
					charaActionResult.targetAnim,
					charaActionResult.currentFadeDuration
				);
			}
		}
	}
}
