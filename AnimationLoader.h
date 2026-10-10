#pragma once

#include <memory>
#include <string>

#include "AnimationClip.h"

namespace Engine
{
	class AnimationLoader
	{
	public:
		static std::shared_ptr<AnimationClip> Load(const std::string& filePath);
	};
}
