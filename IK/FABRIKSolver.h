#pragma once

#include <memory>
#include <unordered_map>

#include "IIKSolver.h"

namespace Engine
{
	class FABRIKSolver : public IIKSolver
	{
	public:
		FABRIKSolver(int maxIterations, float tolerance);
		~FABRIKSolver();

		void Solve(
			std::vector<Bone>& bones,
			const IKComponent& ikData,
			const DirectX::XMFLOAT3& targetPosition
		) override;

	private:
		// A. 位置の解決（一直線化、またはイテレーションによる各関節のグローバル座標の決定）
		void ResolvePositions(
			const DirectX::XMVECTOR& targetPosVec,
			const DirectX::XMVECTOR& rootPosVec,
			std::vector<DirectX::XMVECTOR>& tempPositions,
			const std::vector<float>& boneDistances
		);
		// B. 確定した位置から回転を逆算し、実際のモデルのボーンに適用する
		void ApplyPositionsToBones(
			const IKComponent& ikData,
			const std::vector<DirectX::XMVECTOR>& tempPositions,
			std::vector<Bone>& bones,
			int numJoints
		);

		void UpdateChainFK(
			std::vector<Bone>& bones,
			int boneIndex,
			DirectX::XMMATRIX parentGlobalTransform
		);
		float CalcDistance(DirectX::XMFLOAT4X4& startMat, DirectX::XMFLOAT4X4& endMat);
		float ExtractAngleAroundAxis(DirectX::XMVECTOR newRot, DirectX::XMVECTOR localAxis);

	private:
		int m_maxIterations;
		float m_tolerance;
	};
}