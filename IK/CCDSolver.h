#pragma once
#include "IIKSolver.h"
#include <memory>
#include <unordered_map>

namespace Engine
{
	class CCDSolver : public IIKSolver
	{
	public:
		CCDSolver(int maxIterations, float tolerance);
		~CCDSolver();

	public:
		void Solve(
			std::vector<Bone>& bones,
			const IKComponent& ikData,
			const DirectX::XMFLOAT3& targetPosition
		) override;

		void UpdateChainFK(std::vector<Bone>& bones, int boneIndex, DirectX::XMMATRIX parentGlobalTransform);

	private:
		float ExtractAngleAroundAxis(DirectX::XMVECTOR newRot, DirectX::XMVECTOR localAxis);

	private:
		int m_maxIterations;
		float m_tolerance;
	};
}