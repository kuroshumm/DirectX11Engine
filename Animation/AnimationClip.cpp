#include "AnimationClip.h"

namespace Engine
{
    AnimationClip::AnimationClip(
        float duration,
        float tickPerSecond,
        std::vector<NodeAnimation> nodeAnimations,
        std::unordered_map<std::string, int> nameToChannelMap) 
        : m_Duration(duration),
          m_TicsPerSecond(tickPerSecond),
          m_NodeAnimations(std::move(nodeAnimations)),
          m_nameToChannelMap(std::move(nameToChannelMap))
    {
        assert(m_Duration > 0.0f);
        assert(m_TicsPerSecond > 0.0f);
    }

    AnimationClip::~AnimationClip()
    {

    }
}
