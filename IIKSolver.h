#pragma once
#include <vector>
#include "Common.h"

namespace Engine
{
	class IIKSolver
	{
	public:
		virtual ~IIKSolver() = default;
		
		virtual void SetConstraint(
			int boneIndex,
			DirectX::XMVECTOR localAxis,
			float minAngleDeg, 
			float maxAngleDeg
		) = 0;

		virtual void Solve(
			const int targetIndex,
			const std::vector<int>& boneChainIndices,
			const DirectX::XMFLOAT3& targetPosition
		) = 0;

		struct JointConstraint
		{
			bool isLimited;
			DirectX::XMVECTOR localAxis;
			float minAngleRad;
			float maxAngleRad;
		};
	};
}