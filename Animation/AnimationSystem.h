#pragma once

#include <entt/entt.hpp>

#include "Components.h"

namespace Engine
{
    struct BoneTransformTRS
    {
        DirectX::XMVECTOR scale;
        DirectX::XMVECTOR rot;
        DirectX::XMVECTOR pos;
    };

	class AnimationSystem
	{
	public:
		void Update(entt::registry& registry, float dt);
        BoneTransformTRS SampleBoneTransform(
            const std::shared_ptr<AnimationClip>& clip,
            float time,
            const Bone& bone);

	private:
        template<typename keyType>
        static void CalcInterpolationFactor(
            float currentTime,
            const std::vector<keyType>& keys, // Åö éQè∆ìnÇµ & Ç…èCê≥
            size_t& outLeftIndex,
            size_t& outRightIndex,
            float& outF)
        {
            if (keys.size() < 2)
            {
                outLeftIndex = 0;
                outRightIndex = 0;
                outF = 0.0f;
                return;
            }
            if (currentTime <= keys.front().Time)
            {
                outLeftIndex = 0;
                outRightIndex = 1;
                outF = 0.0f;
                return;
            }
            if (currentTime >= keys.back().Time)
            {
                outLeftIndex = keys.size() - 2;
                outRightIndex = keys.size() - 1;
                outF = 1.0f;
                return;
            }

            // ìÒï™íTçı
            auto it = std::upper_bound(keys.begin(), keys.end(), currentTime,
                [](float time, const auto& key)
                {
                    return time < key.Time;
                }
            );

            outRightIndex = std::distance(keys.begin(), it);
            outLeftIndex = outRightIndex - 1;

            float timeDiff = keys[outRightIndex].Time - keys[outLeftIndex].Time;
            outF = timeDiff > 0.0001f
                ? (currentTime - keys[outLeftIndex].Time) / timeDiff
                : 0.0f;
        }
    };
}