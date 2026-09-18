#pragma once
#include <vector>
#include "Common.h"

namespace Engine
{
	struct IKComponent;

	class IIKSolver
	{
	public:
		virtual ~IIKSolver() = default;
		
		virtual void Solve(
			std::vector<Bone>& bones,
			const IKComponent& ikData,
			const DirectX::XMFLOAT3& targetPosition
		) = 0;
	};
}