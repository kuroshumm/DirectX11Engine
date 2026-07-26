#pragma once
#include "IIKSolver.h"
#include "SkinnedModel.h"
#include <memory>
#include <unordered_map>

namespace Engine
{
	class CCDSolver : public IIKSolver
	{
	public:
		CCDSolver(int maxIterations, float tolerance, std::weak_ptr<SkinnedModel> pModel);
		~CCDSolver();

	public:
		void SetConstraint(
			int boneIndex,
			DirectX::XMVECTOR localAxis,
			float minAngleDeg,
			float maxAngleDeg
		) override;

		void Solve(
			const int targetIndex,
			const std::vector<int>& boneChainIndices,
			const DirectX::XMFLOAT3& targetPosition
		) override;

		void UpdateChainFK(int boneIndex, DirectX::XMMATRIX parentGlobalTransform);

	private:
		float ExtractAngleAroundAxis(DirectX::XMVECTOR newRot, DirectX::XMVECTOR localAxis);

	private:
		std::weak_ptr<SkinnedModel> m_pModel;
		std::unordered_map<int, JointConstraint> m_constraints;
		int m_maxIterations;
		float m_tolerance;
	};
}