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

	const NodeAnimation& AnimationClip::GetNodeAnimation(std::string& nodeName) const
	{
		static const NodeAnimation emptyAnim{};

		auto it = m_NodeAnimations.find(nodeName);
		if (it == m_NodeAnimations.end())
		{
			return emptyAnim;
		}

		const NodeAnimation& nodeAnim = it->second;
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
			// ★重要：同じボーン名で別々に登録されているキーフレーム（TranslationとRotationなど）を合流マージする
			auto it = m_NodeAnimations.find(boneName);
			if (it != m_NodeAnimations.end())
			{
				if (!nodeAnim.PositionKeys.empty()) it->second.PositionKeys = nodeAnim.PositionKeys;
				if (!nodeAnim.ScalingKeys.empty())  it->second.ScalingKeys = nodeAnim.ScalingKeys;
				if (!nodeAnim.RotaionKeys.empty())  it->second.RotaionKeys = nodeAnim.RotaionKeys;
			}
			else
			{
				m_NodeAnimations[boneName] = nodeAnim;
			}
		}

		return S_OK;
	}

}