#include "AnimationClip.h"

namespace Engine
{
	AnimationClip::AnimationClip(const std::string& filePath)
	{
		HRESULT hr = LoadFromFile(filePath);
		assert(SUCCEEDED(hr));
	}
	AnimationClip::~AnimationClip()
	{

	}

	const NodeAnimation& AnimationClip::GetNodeAnimation(size_t index) const
	{
		static const NodeAnimation emptyAnim{};

		if (m_NodeAnimations.size() <= index)
		{
			return emptyAnim;
		}

		const NodeAnimation& nodeAnim = m_NodeAnimations[index];

		return nodeAnim;
	}

	HRESULT AnimationClip::LoadFromFile(const std::string& filePath)
	{
		Assimp::Importer importer;

		//unsigned int flags = aiProcess_MakeLeftHanded;
		unsigned int flags = aiProcess_Triangulate
			 | aiProcess_ConvertToLeftHanded
			 | aiProcess_CalcTangentSpace
			 | aiProcess_GenSmoothNormals;
		const aiScene* scene = importer.ReadFile(filePath, flags);

		if (!scene || !scene->mRootNode || scene->mNumAnimations == 0 || !scene->mAnimations)
		{
			OutputDebugStringA(importer.GetErrorString());
			return E_FAIL;
		}

		// アニメーションの取得
		aiAnimation* anim = scene->mAnimations[0];

		m_Duration = anim->mDuration;
		//m_TicsPerSecond = (anim->mTicksPerSecond != 0.0f) ? anim->mTicksPerSecond : 25.0f;
		m_TicsPerSecond = anim->mTicksPerSecond;

		for (UINT channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex)
		{
			NodeAnimation nodeAnim;
				
			aiNodeAnim* channel = anim->mChannels[channelIndex];

			// スケールの取得
			for (UINT ScaleIndex = 0; ScaleIndex < channel->mNumScalingKeys; ++ScaleIndex)
			{
				aiVectorKey ScaleKey = channel->mScalingKeys[ScaleIndex];
				KeyVector3 key;
				key.Time = ScaleKey.mTime;
				key.Value = { ScaleKey.mValue.x, ScaleKey.mValue.y, ScaleKey.mValue.z };
				nodeAnim.ScalingKeys.push_back(key);
			}

			// 位置の取得
			for (UINT posIndex = 0; posIndex < channel->mNumPositionKeys; ++posIndex)
			{
				aiVectorKey posKey = channel->mPositionKeys[posIndex];
				KeyVector3 key;
				key.Time = posKey.mTime;
				key.Value = { posKey.mValue.x, posKey.mValue.y, posKey.mValue.z };
				nodeAnim.PositionKeys.push_back(key);
			}

			// 回転の取得
			for (UINT rotIndex = 0; rotIndex < channel->mNumRotationKeys; ++rotIndex)
			{
				aiQuatKey rotKey = channel->mRotationKeys[rotIndex];
				KeyQuaternion key;
				key.Time = rotKey.mTime;
				key.Value = { rotKey.mValue.x, rotKey.mValue.y, rotKey.mValue.z, rotKey.mValue.w };
				nodeAnim.RotaionKeys.push_back(key);
			}

			// ★修正：チャンネル名からダミーサフィックスをトリミングする
			std::string boneName = channel->mNodeName.C_Str();
			size_t pos = boneName.find("_$AssimpFbx$_");
			if (pos != std::string::npos)
			{
				boneName = boneName.substr(0, pos); // 例: "LeftArm_$AssimpFbx$_Rotation" -> "LeftArm"
			}

			// 既に登録済みのボーン名か判定
			auto it = m_nameToChannelMap.find(boneName);
			if (it != m_nameToChannelMap.end())
			{
				// ★ 既に存在するボーンなので、該当インデックスにキーを合体マージ！
				int existingIndex = it->second;
				if (!nodeAnim.PositionKeys.empty()) m_NodeAnimations[existingIndex].PositionKeys = std::move(nodeAnim.PositionKeys);
				if (!nodeAnim.ScalingKeys.empty())  m_NodeAnimations[existingIndex].ScalingKeys = std::move(nodeAnim.ScalingKeys);
				if (!nodeAnim.RotaionKeys.empty())  m_NodeAnimations[existingIndex].RotaionKeys = std::move(nodeAnim.RotaionKeys);
			}
			else
			{
				// ★ 新規ボーンなので、インデックスを登録して末尾に追加
				int newIndex = static_cast<int>(m_NodeAnimations.size());
				m_nameToChannelMap[boneName] = newIndex;
				m_NodeAnimations.emplace_back(std::move(nodeAnim));
			}
			
		}

		return S_OK;
	}

}