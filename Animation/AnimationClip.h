#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "Common.h"

namespace Engine
{
	class AnimationClip
	{
	public:
		AnimationClip(
			float duration,
			float tickPerSecond,
			std::vector<NodeAnimation> nodeAnimations,
			std::unordered_map<std::string, int> nameToChannelMap);
		~AnimationClip();

	public:
		const float GetDuration() const { return m_Duration; }
		const float GetTicksPerSecond() const { return m_TicsPerSecond; }
		const NodeAnimation& GetNodeAnimation(size_t index) const
		{
			static const NodeAnimation emptyAnim{};

			if (m_NodeAnimations.size() <= index)
			{
				return emptyAnim;
			}

			const NodeAnimation& nodeAnim = m_NodeAnimations[index];

			return nodeAnim;
		}

		// 名前からチャンネル番号を取得（初期化バインド時に 1 回だけ使用）
		int FindChannelIndex(const std::string& nodeName) const
		{
			auto it = m_nameToChannelMap.find(nodeName);
			return (it != m_nameToChannelMap.end()) ? it->second : -1;
		}

	private:
		float m_Duration;      // アニメーションの総時間（Duration）
		float m_TicsPerSecond; // 1秒あたりのティック数（再生速度基準）

		// 各ボーンのキーフレームアニメーション配列と名前逆引きマップ
		std::vector<NodeAnimation> m_NodeAnimations;
		std::unordered_map<std::string, int> m_nameToChannelMap;
	};
}
