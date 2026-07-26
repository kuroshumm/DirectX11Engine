#include "Animator.h"

namespace Engine
{
	Animator::Animator(std::shared_ptr<SkinnedModel>& pModel)
	{
		m_pModel = pModel;
		m_stateMap.clear();
		m_currentStateName = "";
		m_currentTime = 0;
	}
	Animator::~Animator() 
	{
	}

	void Animator::Update(float deltaTime)
	{
		if (m_currentStateName.empty())
		{
			return;
		}

		auto it = m_stateMap.find(m_currentStateName);
		if (it == m_stateMap.end())
		{
			return;
		}

		const AnimationState& animState = it->second;
		if (!animState.Clip)
		{
			return;
		}

		std::shared_ptr<AnimationClip> clip = animState.Clip;
		for (Bone& bone : m_pModel->GetBone())
		{
			std::string boneName = bone.name;
			const NodeAnimation& nodeAnim = clip->GetNodeAnimation(boneName);

			size_t leftIndex = 0;
			size_t rightIndex = 0;
			float f = 0;

			// ★追加：位置・スケール・回転のキーがすべて存在しない場合は、
			// 初期姿勢（バインドポーズ）を維持するため、このボーンの更新をスキップする
			if (nodeAnim.PositionKeys.empty() &&
				nodeAnim.ScalingKeys.empty() &&
				nodeAnim.RotaionKeys.empty())
			{
				continue;
			}

			// 初期ポーズ行列から初期スケール・回転・位置を分解抽出する
			DirectX::XMMATRIX defaultLocalMatrix = DirectX::XMLoadFloat4x4(&bone.DefaultLocalMatrix);
			DirectX::XMVECTOR defaultScale, defaultRot, defaultPos;
			DirectX::XMMatrixDecompose(&defaultScale, &defaultRot, &defaultPos, defaultLocalMatrix);

			// 1. 平行移動 (Position) の補間・ガード処理
			DirectX::XMVECTOR pos;
			const auto& posKeys = nodeAnim.PositionKeys;
			if (posKeys.empty())
			{
				pos = defaultPos; // キーがない場合は原点
			}
			else if (posKeys.size() == 1)
			{
				pos = DirectX::XMLoadFloat3(&posKeys[0].Value); // キーが1つの場合はその値
			}
			else
			{
				size_t leftIndex = 0, rightIndex = 0;
				float f = 0.0f;
				CalcInterpolationFactor(posKeys, leftIndex, rightIndex, f);
				DirectX::XMVECTOR prev = DirectX::XMLoadFloat3(&posKeys[leftIndex].Value);
				DirectX::XMVECTOR next = DirectX::XMLoadFloat3(&posKeys[rightIndex].Value);
				pos = DirectX::XMVectorLerp(prev, next, f);
			}
			// 2. スケール (Scaling) の補間・ガード処理
			DirectX::XMVECTOR scale;
			const auto& scaleKeys = nodeAnim.ScalingKeys;
			if (scaleKeys.empty())
			{
				scale = defaultScale; // キーがない場合は等倍(1, 1, 1)
			}
			else if (scaleKeys.size() == 1)
			{
				scale = DirectX::XMLoadFloat3(&scaleKeys[0].Value);
			}
			else
			{
				size_t leftIndex = 0, rightIndex = 0;
				float f = 0.0f;
				CalcInterpolationFactor(scaleKeys, leftIndex, rightIndex, f);
				DirectX::XMVECTOR prev = DirectX::XMLoadFloat3(&scaleKeys[leftIndex].Value);
				DirectX::XMVECTOR next = DirectX::XMLoadFloat3(&scaleKeys[rightIndex].Value);
				scale = DirectX::XMVectorLerp(prev, next, f);
			}
			// 3. 回転 (Rotation) の補間・ガード処理
			DirectX::XMVECTOR rot;
			const auto& rotKeys = nodeAnim.RotaionKeys;
			if (rotKeys.empty())
			{
				rot = defaultRot; // キーがない場合は回転なし（単位クォータニオン）
			}
			else if (rotKeys.size() == 1)
			{
				rot = DirectX::XMLoadFloat4(&rotKeys[0].Value);
			}
			else
			{
				size_t leftIndex = 0, rightIndex = 0;
				float f = 0.0f;
				CalcInterpolationFactor(rotKeys, leftIndex, rightIndex, f);
				DirectX::XMVECTOR prev = DirectX::XMLoadFloat4(&rotKeys[leftIndex].Value);
				DirectX::XMVECTOR next = DirectX::XMLoadFloat4(&rotKeys[rightIndex].Value);
				rot = DirectX::XMQuaternionSlerp(prev, next, f);
			}

			DirectX::XMMATRIX localMatrix = DirectX::XMMatrixAffineTransformation(scale, DirectX::XMVectorZero(), rot, pos);

			DirectX::XMStoreFloat4x4(&bone.LocalTransformMatrix, localMatrix);
		}

		UpdateTimer(deltaTime * clip->GetTicksPerSecond(), clip->GetDuration());
	}

	void Animator::Play(std::string stateName)
	{
		m_currentStateName = stateName;
		m_currentTime = 0;
	}

	void Animator::AddState(const std::string& stateName, const std::shared_ptr<AnimationClip>& pClip, bool isLoop)
	{
		AnimationState animState;
		animState.Name = stateName;
		animState.Clip = pClip;
		animState.isLoop = isLoop;

		auto[it, inserted] = m_stateMap.try_emplace(stateName, std::move(animState));
		assert(inserted);
	}


	void Animator::UpdateTimer(float deltaTime, float duration)
	{
		m_currentTime += deltaTime;
		
		// 制限時間を超えたら余りを計算してループさせる
		if (duration > 0.0f)
		{
			m_currentTime = std::fmod(m_currentTime, duration);
		}
	}

	template<typename keyType>
	void Animator::CalcInterpolationFactor(
		const std::vector<keyType> keys,
		size_t& outLeftIndex,
		size_t& outRightIndex,
		float& outF) const
	{
		// ２つのキーを補間するためキーが二つしかない場合は何もしない
		if (keys.size() < 2)
		{
			outLeftIndex = 0;
			outRightIndex = 0;
			outF = 0;
			return;
		}

		// 現在時間が最初のキーより前の場合
		if (m_currentTime <= keys.front().Time)
		{
			outLeftIndex = 0;
			outRightIndex = 1;
			outF = 0.0f;
			return;
		}

		// 現在時間が最後のキーより後ろの場合
		if (m_currentTime >= keys.back().Time)
		{
			outLeftIndex = keys.size() - 2;
			outRightIndex = keys.size() - 1;
			outF = 1.0f;
			return;
		}

		// 中間領域の探索
		for (size_t index = 0; index < keys.size() - 1; ++index)
		{
			if (m_currentTime < keys[index + 1].Time)
			{
				outLeftIndex = index;
				outRightIndex = index + 1;
				break;
			}
		}

		// 
		float timeDiff = keys[outRightIndex].Time - keys[outLeftIndex].Time;
		outF = timeDiff > 0.0001f
			? (m_currentTime - keys[outLeftIndex].Time) / timeDiff
			: 0.0f;
	}
}