#include <windows.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/anim.h>

#include "AnimationLoader.h"

namespace Engine
{
    std::shared_ptr<AnimationClip> AnimationLoader::Load(const std::string& filePath)
    {
        Assimp::Importer importer;

        unsigned int flags = aiProcess_ConvertToLeftHanded;
        const aiScene* scene = importer.ReadFile(filePath, flags);
        if (!scene || !scene->mRootNode || scene->mNumAnimations == 0 || !scene->mAnimations)
        {
            return nullptr;
        }

        // アニメーションの取得
        aiAnimation* anim = scene->mAnimations[0];

        if (anim->mDuration <= 0.0 || anim->mTicksPerSecond <= 0.0)
        {
            assert(false && "Invalid animation duration or ticksPerSecond in asset.");
            return nullptr;
        }

        float duration = static_cast<float>(anim->mDuration);
        float ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);

        std::vector<NodeAnimation> nodeAnimations;
        std::unordered_map<std::string, int> nameToChannelMap;

        for (UINT channelIndex = 0; channelIndex < anim->mNumChannels; ++channelIndex)
        {
            NodeAnimation nodeAnim;

            aiNodeAnim* channel = anim->mChannels[channelIndex];

            // スケールの取得
            for (UINT ScaleIndex = 0; ScaleIndex < channel->mNumScalingKeys; ++ScaleIndex)
            {
                aiVectorKey ScaleKey = channel->mScalingKeys[ScaleIndex];
                KeyVector3 key;
                key.Time = static_cast<float>(ScaleKey.mTime);
                key.Value = { static_cast<float>(ScaleKey.mValue.x), static_cast<float>(ScaleKey.mValue.y), static_cast<float>(ScaleKey.mValue.z) };
                nodeAnim.ScalingKeys.push_back(key);
            }

            // 位置の取得
            for (UINT posIndex = 0; posIndex < channel->mNumPositionKeys; ++posIndex)
            {
                aiVectorKey posKey = channel->mPositionKeys[posIndex];
                KeyVector3 key;
                key.Time = static_cast<float>(posKey.mTime);
                key.Value = { static_cast<float>(posKey.mValue.x), static_cast<float>(posKey.mValue.y), static_cast<float>(posKey.mValue.z) };
                nodeAnim.PositionKeys.push_back(key);
            }

            // 回転の取得
            for (UINT rotIndex = 0; rotIndex < channel->mNumRotationKeys; ++rotIndex)
            {
                aiQuatKey rotKey = channel->mRotationKeys[rotIndex];
                KeyQuaternion key;
                key.Time = static_cast<float>(rotKey.mTime);
                key.Value = { static_cast<float>(rotKey.mValue.x), static_cast<float>(rotKey.mValue.y), static_cast<float>(rotKey.mValue.z), static_cast<float>(rotKey.mValue.w) };
                nodeAnim.RotaionKeys.push_back(key);
            }

            // ★修正：チャンネル名からダミーサフィックスをトリミングする
            std::string boneName = channel->mNodeName.C_Str();
            size_t pos = boneName.find("_$AssimpFbx$_");
            if (pos != std::string::npos)
            {
                boneName = boneName.substr(0, pos); // 例: "LeftArm_$AssimpFbx$_Rotation" -> "LeftArm"
            }

            // 既に登録済みのボーン名か判定
            auto it = nameToChannelMap.find(boneName);
            if (it != nameToChannelMap.end())
            {
                // ★ 既に存在するボーンなので、該当インデックスにキーフレームをマージ！
                int existingIndex = it->second;
                if (!nodeAnim.PositionKeys.empty()) nodeAnimations[existingIndex].PositionKeys = std::move(nodeAnim.PositionKeys);
                if (!nodeAnim.ScalingKeys.empty())  nodeAnimations[existingIndex].ScalingKeys = std::move(nodeAnim.ScalingKeys);
                if (!nodeAnim.RotaionKeys.empty())  nodeAnimations[existingIndex].RotaionKeys = std::move(nodeAnim.RotaionKeys);
            }
            else
            {
                // ★ 新規ボーンなので、インデックスを登録して末尾に追加
                int newIndex = static_cast<int>(nodeAnimations.size());
                nameToChannelMap[boneName] = newIndex;
                nodeAnimations.emplace_back(std::move(nodeAnim));
            }
        }

        return std::make_shared<AnimationClip>(
            duration,
            ticksPerSecond,
            std::move(nodeAnimations),
            std::move(nameToChannelMap)
        );
    }
}
