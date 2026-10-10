#pragma once

#include <memory>
#include <string>
#include "AnimSequenceAsset.h"
#include "Components/AnimationComponents.h"

namespace Engine
{
	class AnimSequenceLoader
	{
	public:
		static std::shared_ptr<AnimSequenceAsset> Load(AnimStateType stateType);
	};
}

