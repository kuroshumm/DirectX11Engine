#pragma once

#include <entt/entt.hpp>
#include <DirectXMath.h>
#include "Components.h"

namespace Engine
{
	class BonePaletteUploadSystem
	{
	public:
		void Update(entt::registry& registry, ID3D11DeviceContext& context);

		void UploadPaletteBuffer(
			ID3D11DeviceContext& context,
			SkeletonComponent& skeltonComp,
			SkinningPaletteComponent& skinningPaletteComp
		);
	};
}