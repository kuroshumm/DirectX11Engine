#pragma once
#include <wrl/client.h>
#include <string>
#include <unordered_map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Common.h"

namespace Engine
{
	class AnimationClip
	{
	public:
		AnimationClip(const std::string& filePath);
		~AnimationClip();

	public:
		const NodeAnimation& GetNodeAnimation(size_t index) const;
		const float GetDuration() const { return m_Duration; }
		const float GetTicksPerSecond() const { return m_TicsPerSecond; }
		// 名前からチャンネル番号を検索（初回バインド時に 1 回だけ使用）
		int FindChannelIndex(const std::string& nodeName) const
		{
			auto it = m_nameToChannelMap.find(nodeName);
			return (it != m_nameToChannelMap.end()) ? it->second : -1;
		}

	private:
		HRESULT LoadFromFile(const std::string& filePath);

	private:
		float m_Duration;      // アニメーションの総持続時間
		float m_TicsPerSecond; // １秒あたりのティック数（再生速度基準）

		// ボーン名をキーにして、各ボーンのキーフレーム配列を検索・保持するマップ
		//std::unordered_map<std::string, NodeAnimation> m_NodeAnimations;
		std::vector<NodeAnimation> m_NodeAnimations;
		std::unordered_map<std::string, int> m_nameToChannelMap;
	};
}