#pragma once

#include <vector>
#include <memory>

#include "AnimationEventType.h"
#include "AnimStateType.h"

namespace Engine
{
	class AnimationClip;

	struct AnimSequenceAsset
	{
		AnimStateType animState = AnimStateType::NONE;
		std::shared_ptr<AnimationClip> clip = nullptr;
		std::vector<AnimWindowEvent> windowEvents;
		std::vector<AnimPointEvent> pointEvents;
	};
}
