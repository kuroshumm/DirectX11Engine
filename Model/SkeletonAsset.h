#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "SkeletonTypes.h"

namespace Engine
{
    // Shared immutable skeleton asset definition
    struct SkeltonAsset
    {
        std::vector<Bone> initialBones;
        int rootBoneIndex = -1;
        std::unordered_map<std::string, int> boneMapping;

        int GetBoneIndex(const std::string& name) const
        {
            auto it = boneMapping.find(name);
            if (it != boneMapping.end())
            {
                return it->second;
            }
            return -1;
        }
    };
}
