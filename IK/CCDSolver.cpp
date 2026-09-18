#include "CCDSolver.h"
#include "Components.h"

namespace Engine
{
	CCDSolver::CCDSolver(int maxIterations, float tolerance)
	{
		m_maxIterations = maxIterations;
		m_tolerance = tolerance;
	}
	CCDSolver::~CCDSolver()
	{

	}

	void CCDSolver::Solve(
		std::vector<Bone>& bones,
		const IKComponent& ikData,
		const DirectX::XMFLOAT3& targetPosition
	)
	{
		Bone& targetBone = bones[ikData.effectorBoneIndex];
		DirectX::XMVECTOR targetPosVec = DirectX::XMLoadFloat3(&targetPosition);

		// 
		for (size_t iter = 0; iter < m_maxIterations; ++iter)
		{
			// 
			for (int index = ikData.chainIndices.size() - 2; index >= 0; --index)
			{
				size_t boneIndex = ikData.chainIndices[index];
				Bone& bone = bones[boneIndex];
				DirectX::XMMATRIX globalMatrix = DirectX::XMLoadFloat4x4(&bone.GlobalTransformMatrix);
				DirectX::XMVECTOR bonePosVec = globalMatrix.r[3];

				// エフェクタ（末端）のBoneの座標を取得
				DirectX::XMMATRIX effectorGlobalMatrix = DirectX::XMLoadFloat4x4(&targetBone.GlobalTransformMatrix);
				DirectX::XMVECTOR effectorBonePosVec = effectorGlobalMatrix.r[3];

				// 注目関節⇒エフェクタ（末端）へのベクトル
				DirectX::XMVECTOR toEffectVec = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(effectorBonePosVec, bonePosVec));
				// 注目関節⇒ターゲットへのベクトル
				DirectX::XMVECTOR toTargetVec = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(targetPosVec, bonePosVec));

				// ターゲットに到達するための回転軸
				DirectX::XMVECTOR axis = DirectX::XMVector3Cross(toEffectVec, toTargetVec);
				DirectX::XMMATRIX globalMat = DirectX::XMLoadFloat4x4(&bone.GlobalTransformMatrix);
				DirectX::XMVECTOR globalScale, globalRot, globalTransform;
				DirectX::XMMatrixDecompose(&globalScale, &globalRot, &globalTransform, globalMat);
				DirectX::XMVECTOR localAxis = DirectX::XMVector3Rotate(axis, DirectX::XMQuaternionInverse(globalRot));
				localAxis = DirectX::XMVector3Normalize(localAxis);
				// ターゲットに到達するための角度
				DirectX::XMVECTOR cosValue = DirectX::XMVector3Dot(toEffectVec, toTargetVec);
				DirectX::XMVECTOR clampedCos = DirectX::XMVectorClamp(cosValue, DirectX::XMVectorReplicate(-1.0f), DirectX::XMVectorReplicate(1.0f));
				DirectX::XMVECTOR angleRadianVec = DirectX::XMVectorACos(clampedCos);
				float radianAngle = DirectX::XMVectorGetX(angleRadianVec);

				// ターゲットに到達するためのクォータニオン
				DirectX::XMVECTOR deltaRot = DirectX::XMQuaternionRotationNormal(localAxis, radianAngle);

				// 注目関節のローカル行列を分解
				DirectX::XMMATRIX localMat = DirectX::XMLoadFloat4x4(&bone.LocalTransformMatrix);
				DirectX::XMVECTOR scale, rot, transform;
				DirectX::XMMatrixDecompose(&scale, &rot, &transform, localMat);

				// 回転
				DirectX::XMVECTOR newRot = DirectX::XMQuaternionMultiply(rot, deltaRot);
				
				// 回転を制限内に収める
				auto it = ikData.constraints.find(static_cast<int>(boneIndex));
				if (it != ikData.constraints.end())
				{
					const JointConstraint& constraint = it->second;
					// クォータニオンから、制限したい特定のローカル軸（ピッチ角など）の角度を抽出する
					// (※ DirectXMath のオイラー角変換や、Swing-Twist分解などの幾何学計算を用います)
					float currentAngle = ExtractAngleAroundAxis(newRot, constraint.localAxis);

					// 角度を許容範囲内にクランプ
					float clampedAngle = std::clamp(currentAngle, constraint.minAngleRad, constraint.maxAngleRad);
					// 制限された角度と軸から、ブレのないクォータニオンを再生成して上書きする
					newRot = DirectX::XMQuaternionRotationAxis(constraint.localAxis, clampedAngle);
				}

				// 回転した後のローカル行列を再構築
				DirectX::XMMATRIX newLocalMat = DirectX::XMMatrixAffineTransformation(
					scale,
					DirectX::XMVectorZero(),
					newRot,
					transform
				);

				DirectX::XMStoreFloat4x4(&bone.LocalTransformMatrix, newLocalMat);

				DirectX::XMMATRIX parentGlobalMat = DirectX::XMMatrixIdentity();
				if (bone.parentIndex != -1)
				{
					parentGlobalMat = DirectX::XMLoadFloat4x4(&bones[bone.parentIndex].GlobalTransformMatrix);
				}

				UpdateChainFK(bones, boneIndex, parentGlobalMat);
			}

			// エフェクタ（末端）の位置がターゲットの位置に近づいたら終了
			DirectX::XMMATRIX effectorGlobalMatrix = DirectX::XMLoadFloat4x4(&targetBone.GlobalTransformMatrix);
			DirectX::XMVECTOR effectorBonePosVec = effectorGlobalMatrix.r[3];
			DirectX::XMVECTOR diffVec = DirectX::XMVectorSubtract(targetPosVec, effectorBonePosVec);
			DirectX::XMVECTOR lengthVec = DirectX::XMVector3Length(diffVec);
			float distance = DirectX::XMVectorGetX(lengthVec);
			if (distance < m_tolerance)
			{
				break;
			}
		}
	}

	void CCDSolver::UpdateChainFK(std::vector<Bone>& bones, int boneIndex, DirectX::XMMATRIX parentGlobalTransform)
	{
		Bone& targetBone = bones[boneIndex];

		DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&targetBone.LocalTransformMatrix);
		DirectX::XMMATRIX global = DirectX::XMMatrixMultiply(local, parentGlobalTransform);
		DirectX::XMStoreFloat4x4(&targetBone.GlobalTransformMatrix, global);

		for (int childIndex : targetBone.childIndices)
		{
			UpdateChainFK(bones, childIndex, global);
		}
	}


	float CCDSolver::ExtractAngleAroundAxis(DirectX::XMVECTOR newRot, DirectX::XMVECTOR localAxis)
	{
		// q = (x, y, z, w), axis = (ax, ay, az, 0)
		// ※ axis はあらかじめ正規化（長さ 1.0）されている必要があります。
		// 
		// 1. クォータニオンのベクトル部 (x, y, z) と指定軸の内積を計算する
		// これにより、指定軸方向への回転成分の大きさ（sin(theta/2) に比例）が抽出されます
		DirectX::XMVECTOR dotVec = DirectX::XMVector3Dot(newRot, localAxis);
		float dot = DirectX::XMVectorGetX(dotVec);
		// 2. クォータニオンの W 成分 (cos(theta/2) に対応) を取得する
		float w = DirectX::XMVectorGetW(newRot);
		// 3. atan2 を用いて、軸まわりの半角 (theta/2) を算出する
		float halfAngle = std::atan2(dot, w);
		// 4. 半角を2倍して実際の回転角 theta を得る
		float angle = 2.0f * halfAngle;
		// 5. 角度の範囲を [-pi, pi] の範囲に正規化する（安全対策）
		if (angle > DirectX::XM_PI)  angle -= DirectX::XM_2PI;
		if (angle < -DirectX::XM_PI) angle += DirectX::XM_2PI;
		return angle;
	}

}