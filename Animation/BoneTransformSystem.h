#pragma once

#include <entt/entt.hpp>
#include <DirectXMath.h>
#include "Components.h"

namespace Engine
{
	class BoneTransformSystem
	{
	public:
		void Update(entt::registry& registry);

	private:
		void UpdateBoneTransform(
			std::vector<Bone>& bones,
			int boneIndex,
			DirectX::XMMATRIX parentGlocbalTransform
		);
	};
}