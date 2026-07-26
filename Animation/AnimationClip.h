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
		const NodeAnimation& GetNodeAnimation(std::string& nodeName) const;
		const float GetDuration() const { return m_Duration; }
		const float GetTicksPerSecond() const { return m_TicsPerSecond; }

	private:
		HRESULT LoadFromFile(const std::string& filePath);

	private:
		float m_Duration;      // アニメーションの総持続時間
		float m_TicsPerSecond; // １秒あたりのティック数（再生速度基準）

		// ボーン名をキーにして、各ボーンのキーフレーム配列を検索・保持するマップ
		std::unordered_map<std::string, NodeAnimation> m_NodeAnimations;
	};
}