#include "FABRIKSolver.h"

namespace Engine
{
	FABRIKSolver::FABRIKSolver(int maxIterations, float tolerance, std::weak_ptr<SkinnedModel> pModel)
	{
		m_maxIterations = maxIterations;
		m_tolerance = tolerance;
		m_pModel = pModel;
	}
	FABRIKSolver::~FABRIKSolver()
	{

	}

	void FABRIKSolver::SetConstraint(int boneIndex, DirectX::XMVECTOR localAxis, float minAngleDeg, float maxAngleDeg)
	{
		JointConstraint constraint;
		constraint.isLimited = true;
		constraint.localAxis = DirectX::XMVector3Normalize(localAxis); // 軸を確実に正規化
		constraint.minAngleRad = DirectX::XMConvertToRadians(minAngleDeg);
		constraint.maxAngleRad = DirectX::XMConvertToRadians(maxAngleDeg);
		m_constraints[boneIndex] = constraint;
	}

	void FABRIKSolver::Solve(
		const int targetIndex,
		const std::vector<int>& boneChainIndices,
		const DirectX::XMFLOAT3& targetPosition
	)
	{
		std::shared_ptr<SkinnedModel> model = m_pModel.lock();
		if (model == nullptr) return;

		std::vector<Bone>& bones = model->GetBone();
		int numJoints = (int)boneChainIndices.size();

		// 1. 各関節の現在のグローバル座標をロード
		std::vector<DirectX::XMVECTOR> tempPosition(numJoints);
		for (size_t index = 0; index < numJoints; ++index)
		{
			int boneIndex = boneChainIndices[index];
			tempPosition[index] = DirectX::XMLoadFloat4x4(&bones[boneIndex].GlobalTransformMatrix).r[3];
		}

		// 2. バインドポーズでの各ボーン長（初期距離）をキャッシュ
		std::vector<float> boneDistances;
		for (size_t index = 0; index < boneChainIndices.size() - 1; ++index)
		{
			int parentBoneIdx = boneChainIndices[index];
			int childBoneIdx = boneChainIndices[index + 1];
			Bone& parentBone = bones[parentBoneIdx];
			Bone& childBone = bones[childBoneIdx];
			// ローカル行列ではなく、グローバル行列の平行移動成分同士の距離を測る
			DirectX::XMMATRIX parentGlobal = DirectX::XMLoadFloat4x4(&parentBone.GlobalTransformMatrix);
			DirectX::XMMATRIX childGlobal = DirectX::XMLoadFloat4x4(&childBone.GlobalTransformMatrix);
			float boneLength = DirectX::XMVectorGetX(DirectX::XMVector3Length(
				DirectX::XMVectorSubtract(childGlobal.r[3], parentGlobal.r[3])
			));
			boneDistances.push_back(boneLength);
		}

		// 3. 位置の解決およびバインド処理を実行
		Bone& chainRootBone = bones[boneChainIndices[0]];
		DirectX::XMVECTOR rootPos = DirectX::XMLoadFloat4x4(&chainRootBone.GlobalTransformMatrix).r[3];
		DirectX::XMVECTOR targetPosVec = DirectX::XMLoadFloat3(&targetPosition);

		// 一時座標位置の解決
		ResolvePositions(targetPosVec, rootPos, tempPosition, boneDistances);

		// 確定座標から回転を逆算して適用
		ApplyPositionsToBones(boneChainIndices, tempPosition, bones, numJoints);
	}

	void FABRIKSolver::ResolvePositions(
		const DirectX::XMVECTOR& targetPosVec,
		const DirectX::XMVECTOR& rootPosVec,
		std::vector<DirectX::XMVECTOR>& tempPosition,
		const std::vector<float>& boneDistances
	)
	{
		int numJoints = (int)tempPosition.size();
		DirectX::XMVECTOR toTargetVec = DirectX::XMVectorSubtract(targetPosVec, rootPosVec);
		float distanceToTarget = DirectX::XMVectorGetX(DirectX::XMVector3Length(toTargetVec));

		float totalBoneDistance = 0.0f;
		for (float d : boneDistances) totalBoneDistance += d;

		// --- 到達不可能な場合：ターゲットに向けて一直線に伸ばす（早期リターン） ---
		if (distanceToTarget > totalBoneDistance)
		{
			DirectX::XMVECTOR direction = DirectX::XMVector3Normalize(toTargetVec);
			for (size_t index = 0; index < numJoints - 1; ++index) // 範囲外アクセスを防止
			{
				float length = boneDistances[index];
				tempPosition[index + 1] = DirectX::XMVectorMultiplyAdd(
					direction,
					DirectX::XMVectorReplicate(length),
					tempPosition[index]
				);
			}
			return; // 早期リターン
		}

		// --- 到達可能な場合：FABRIKイテレーションループ ---
		for (size_t iter = 0; iter < m_maxIterations; ++iter)
		{
			// 前方到達 (末端 -> 根本)
			tempPosition[numJoints - 1] = targetPosVec;
			for (int index = numJoints - 2; index >= 0; --index)
			{
				DirectX::XMVECTOR toParent = DirectX::XMVectorSubtract(tempPosition[index], tempPosition[index + 1]);
				float len = DirectX::XMVectorGetX(DirectX::XMVector3Length(toParent));
				tempPosition[index] = (len > 0.0001f)
					? DirectX::XMVectorMultiplyAdd(DirectX::XMVector3Normalize(toParent), DirectX::XMVectorReplicate(boneDistances[index]), tempPosition[index + 1])
					: tempPosition[index + 1];
			}

			// 後方到達 (根本 -> 末端)
			tempPosition[0] = rootPosVec;
			for (int index = 0; index < numJoints - 1; ++index)
			{
				DirectX::XMVECTOR toChild = DirectX::XMVectorSubtract(tempPosition[index + 1], tempPosition[index]);
				float len = DirectX::XMVectorGetX(DirectX::XMVector3Length(toChild));
				tempPosition[index + 1] = (len > 0.0001f)
					? DirectX::XMVectorMultiplyAdd(DirectX::XMVector3Normalize(toChild), DirectX::XMVectorReplicate(boneDistances[index]), tempPosition[index])
					: tempPosition[index];
			}

			// 収束判定
			DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(tempPosition[numJoints - 1], targetPosVec);
			if (DirectX::XMVectorGetX(DirectX::XMVector3Length(diff)) < m_tolerance)
			{
				break;
			}
		}
	}

	void FABRIKSolver::ApplyPositionsToBones(
		const std::vector<int>& boneChainIndices,
		const std::vector<DirectX::XMVECTOR>& tempPositions,
		std::vector<Bone>& bones,
		int numJoints
	)
	{
		// イテレーションループ終了後に追加する、位置から回転へのバインド処理
		for (int i = 0; i < numJoints - 1; ++i)
		{
			int boneIdx = boneChainIndices[i];
			int childBoneIdx = boneChainIndices[i + 1];
			Bone& bone = bones[boneIdx];
			Bone& childBone = bones[childBoneIdx];

			// --- ① 現在（FK適用状態）の親子グローバル位置から、元のボーンの向きベクトルを算出 ---
			DirectX::XMMATRIX globalMat = DirectX::XMLoadFloat4x4(&bone.GlobalTransformMatrix);
			DirectX::XMMATRIX childGlobalMat = DirectX::XMLoadFloat4x4(&childBone.GlobalTransformMatrix);
			DirectX::XMVECTOR currentDir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(childGlobalMat.r[3], globalMat.r[3]));

			// --- ② FABRIKで確定した新しいグローバル座標から、新しいボーンの向きベクトルを算出 ---
			DirectX::XMVECTOR newDir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(tempPositions[i + 1], tempPositions[i]));

			// ゼロ除算の防止（ダミーノード等で向きが定義できない場合はこの関節の回転更新をスキップ）
			if (DirectX::XMVector3NearEqual(currentDir, DirectX::XMVectorZero(), DirectX::XMVectorReplicate(0.0001f)) ||
				DirectX::XMVector3NearEqual(newDir, DirectX::XMVectorZero(), DirectX::XMVectorReplicate(0.0001f)))
			{
				continue;
			}

			// --- ③ 元の向きから新しい向きへの差分回転（グローバル基準）を算出 ---
			DirectX::XMVECTOR axis = DirectX::XMVector3Cross(currentDir, newDir);
			DirectX::XMVECTOR dotVec = DirectX::XMVector3Dot(currentDir, newDir);
			float dot = DirectX::XMVectorGetX(dotVec);

			DirectX::XMVECTOR deltaRot;
			if (dot > 0.9999f)
			{
				// ほぼ同じ向きを向いているため、回転は不要（恒等クォータニオン）
				deltaRot = DirectX::XMQuaternionIdentity();
			}
			else if (dot < -0.9999f)
			{
				// 完全に真逆（180度反対）を向いている特異点
				// currentDir と直交する適当な軸（orthoAxis）を求めて 180度 回転する
				DirectX::XMVECTOR orthoAxis = DirectX::XMVector3Orthogonal(currentDir);
				orthoAxis = DirectX::XMVector3Normalize(orthoAxis);
				deltaRot = DirectX::XMQuaternionRotationNormal(orthoAxis, DirectX::XM_PI);
			}
			else
			{
				// 一般的なケース：cosθ から角度を、外積から回転軸を求め、正規化してクォータニオンを構築
				float angle = std::acos(dot);
				DirectX::XMVECTOR normAxis = DirectX::XMVector3Normalize(axis);
				deltaRot = DirectX::XMQuaternionRotationNormal(normAxis, angle);
			}

			// --- ④ 現在のグローバル回転に差分を乗算し、新しいグローバル回転を計算 ---
			DirectX::XMVECTOR currentScale, currentRot, currentTrans;
			DirectX::XMMatrixDecompose(&currentScale, &currentRot, &currentTrans, globalMat);
			// グローバル差分回転を乗算（回転順序に注意）
			DirectX::XMVECTOR newGlobalRot = DirectX::XMQuaternionMultiply(deltaRot, currentRot);

			// --- ⑤ 親ボーンの回転を取り除き、新しいローカル回転を算出 ---
			DirectX::XMVECTOR newLocalRot;
			if (bone.parentIndex != -1)
			{
				DirectX::XMMATRIX parentGlobalMat = DirectX::XMLoadFloat4x4(&bones[bone.parentIndex].GlobalTransformMatrix);
				DirectX::XMVECTOR pScale, pRot, pTrans;
				DirectX::XMMatrixDecompose(&pScale, &pRot, &pTrans, parentGlobalMat);

				// ローカル回転 = 新しいグローバル回転 * 親のグローバル回転の逆クォータニオン
				newLocalRot = DirectX::XMQuaternionMultiply(newGlobalRot, DirectX::XMQuaternionInverse(pRot));
			}
			else
			{
				newLocalRot = newGlobalRot;
			}
			
			// --- ⑥ 角度制約（SetConstraintで設定された可動域制限）の適用 ---
			if (m_constraints.count(boneIdx) > 0)
			{
				const JointConstraint& constraint = m_constraints[boneIdx];

				// CCDSolverで実装した ExtractAngleAroundAxis を使用
				float currentAngle = ExtractAngleAroundAxis(newLocalRot, constraint.localAxis);
				float clampedAngle = std::clamp(currentAngle, constraint.minAngleRad, constraint.maxAngleRad);

				float deltaAngle = clampedAngle - currentAngle;
				DirectX::XMVECTOR diffRot = DirectX::XMQuaternionRotationAxis(constraint.localAxis, deltaAngle);

				// 制限された角度でローカル回転を再合成
				newLocalRot = DirectX::XMQuaternionMultiply(diffRot, newLocalRot);
			}


			// --- ⑦ 新しいローカル行列を再構築して保存 ---
			DirectX::XMMATRIX localMat = DirectX::XMLoadFloat4x4(&bone.LocalTransformMatrix);
			DirectX::XMVECTOR lScale, lRot, lTrans;
			DirectX::XMMatrixDecompose(&lScale, &lRot, &lTrans, localMat);

			DirectX::XMMATRIX newLocalMat = DirectX::XMMatrixAffineTransformation(
				lScale,
				DirectX::XMVectorZero(),
				newLocalRot,
				lTrans
			);
			DirectX::XMStoreFloat4x4(&bone.LocalTransformMatrix, newLocalMat);
			// --- ⑧ 次のボーン (i+1) の「現在の向き」を正しく計算するため、このボーンのFKを即座に更新する ---
			DirectX::XMMATRIX parentGlobal = DirectX::XMMatrixIdentity();
			if (bone.parentIndex != -1)
			{
				parentGlobal = DirectX::XMLoadFloat4x4(&bones[bone.parentIndex].GlobalTransformMatrix);
			}
			UpdateChainFK(boneIdx, parentGlobal);
		}
	}


	void FABRIKSolver::UpdateChainFK(int boneIndex, DirectX::XMMATRIX parentGlobalTransform)
	{
		auto model = m_pModel.lock();
		if (model == nullptr)
		{
			return;
		}
		std::vector<Bone>& bones = model->GetBone();
		Bone& targetBone = bones[boneIndex];

		DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&targetBone.LocalTransformMatrix);
		DirectX::XMMATRIX global = DirectX::XMMatrixMultiply(local, parentGlobalTransform);
		DirectX::XMStoreFloat4x4(&targetBone.GlobalTransformMatrix, global);

		for (int childIndex : targetBone.childIndices)
		{
			UpdateChainFK(childIndex, global);
		}
	}

	float FABRIKSolver::CalcDistance(DirectX::XMFLOAT4X4& startMat, DirectX::XMFLOAT4X4& endMat)
	{
		DirectX::XMVECTOR startPos, startScale, startRot;
		DirectX::XMMATRIX startDefaultLocalMatrix = DirectX::XMLoadFloat4x4(&startMat);
		DirectX::XMMatrixDecompose(&startScale, &startRot, &startPos, startDefaultLocalMatrix);

		DirectX::XMVECTOR endPos, endScale, endRot;
		DirectX::XMMATRIX endDefaultLocalMatrix = DirectX::XMLoadFloat4x4(&endMat);
		DirectX::XMMatrixDecompose(&endScale, &endRot, &endPos, endDefaultLocalMatrix);

		DirectX::XMVECTOR diffVec = DirectX::XMVectorSubtract(startPos, endPos);
		DirectX::XMVECTOR lengthVec = DirectX::XMVector3Length(diffVec);

		float distance = DirectX::XMVectorGetX(lengthVec);

		return distance;
	}


	float FABRIKSolver::ExtractAngleAroundAxis(DirectX::XMVECTOR newRot, DirectX::XMVECTOR localAxis)
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