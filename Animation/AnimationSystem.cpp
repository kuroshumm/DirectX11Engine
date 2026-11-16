#include "AnimationSystem.h"
#include "AnimationClip.h"

namespace Engine
{
    void AnimationSystem::Update(entt::registry& registry, float dt)
    {
        auto entities = registry.view<AnimationComponent, SkeletonComponent, AnimationUpdateTag>();
        for (auto [entity, animationComp, skeltonComp] : entities.each())
        {
            // ① 再生フラグ・クリップ・モデルの存在チェック
            if (!animationComp.isPlaying || !animationComp.pCurrentClip || skeltonComp.rootBoneIndex == -1)
            {
                continue;
            }

            // AnimatorComponent を取得し、クロスフェード遷移中か確認
            const AnimatorComponent* pAnimator = registry.try_get<AnimatorComponent>(entity);
            const bool isBlending = (pAnimator && pAnimator->isTransitioning && pAnimator->pPrevClip);
            const float blendWeight = isBlending ? pAnimator->transitionWeight : 1.0f;
            std::shared_ptr<AnimationClip> currClip = animationComp.pCurrentClip;
            std::shared_ptr<AnimationClip> prevClip = isBlending ? pAnimator->pPrevClip : nullptr;

            // ② 各ボーンに対してキーフレーム補間を適用
            auto& bones = skeltonComp.bones;
            for (Bone& bone : bones)
            {
                // 1. Current クリップの姿勢サンプリング
                BoneTransformTRS currPose = SampleBoneTransform(currClip, animationComp.currentTime, bone);
                DirectX::XMVECTOR finalScale = currPose.scale;
                DirectX::XMVECTOR finalRot = currPose.rot;
                DirectX::XMVECTOR finalPos = currPose.pos;
                // 2. 遷移中なら Prev クリップもサンプリングして Lerp / Slerp 合成
                if (isBlending)
                {
                    BoneTransformTRS prevPose = SampleBoneTransform(prevClip, pAnimator->prevTime, bone);
                    finalScale = DirectX::XMVectorLerp(prevPose.scale, currPose.scale, blendWeight);
                    finalPos = DirectX::XMVectorLerp(prevPose.pos, currPose.pos, blendWeight);
                    finalRot = DirectX::XMQuaternionSlerp(prevPose.rot, currPose.rot, blendWeight);
                }
                // 3. 行列合成と格納
                DirectX::XMMATRIX localMatrix = DirectX::XMMatrixAffineTransformation(
                    finalScale, DirectX::XMVectorZero(), finalRot, finalPos
                );
                DirectX::XMStoreFloat4x4(&bone.LocalTransformMatrix, localMatrix);
            }
        }
    }

    BoneTransformTRS AnimationSystem::SampleBoneTransform(
        const std::shared_ptr<AnimationClip>& clip,
        float time,
        const Bone& bone
    )
    {
        const float ticksPerSec = clip->GetTicksPerSecond();
        const float timeInTicks = time * ticksPerSec;

        DirectX::XMVECTOR defaultScale = DirectX::XMLoadFloat3(&bone.defaultScale);
        DirectX::XMVECTOR defaultRot = DirectX::XMLoadFloat4(&bone.defaultRot);
        DirectX::XMVECTOR defaultPos = DirectX::XMLoadFloat3(&bone.defaultPos);
        if (!clip)
        {
            return { defaultScale, defaultRot, defaultPos };
        }
        // クリップごとに正しいチャンネル番号を安全に検索
        int channelIndex = clip->FindChannelIndex(bone.name);
        if (channelIndex < 0)
        {
            // このクリップに該当ボーンのアニメーションがない場合は初期姿勢を返す
            return { defaultScale, defaultRot, defaultPos };
        }
        const NodeAnimation& nodeAnim = clip->GetNodeAnimation(channelIndex);
        if (nodeAnim.PositionKeys.empty() && nodeAnim.ScalingKeys.empty() && nodeAnim.RotaionKeys.empty())
        {
            return { defaultScale, defaultRot, defaultPos };
        }

        // 1. 移動 (Position)
        DirectX::XMVECTOR pos;
        if (nodeAnim.PositionKeys.empty())
        {
            pos = defaultPos;
        }
        else if (nodeAnim.PositionKeys.size() == 1)
        {
            pos = DirectX::XMLoadFloat3(&nodeAnim.PositionKeys[0].Value);
        }
        else
        {
            size_t left = 0, right = 0; float f = 0.0f;
            CalcInterpolationFactor(timeInTicks, nodeAnim.PositionKeys, left, right, f);
            pos = DirectX::XMVectorLerp(
                DirectX::XMLoadFloat3(&nodeAnim.PositionKeys[left].Value),
                DirectX::XMLoadFloat3(&nodeAnim.PositionKeys[right].Value), f);
        }

        // 2. スケール (Scaling)
        DirectX::XMVECTOR scale;
        if (nodeAnim.ScalingKeys.empty())
        {
            scale = defaultScale;
        }
        else if (nodeAnim.ScalingKeys.size() == 1)
        {
            scale = DirectX::XMLoadFloat3(&nodeAnim.ScalingKeys[0].Value);
        }
        else
        {
            size_t left = 0, right = 0; float f = 0.0f;
            CalcInterpolationFactor(timeInTicks, nodeAnim.ScalingKeys, left, right, f);
            scale = DirectX::XMVectorLerp(
                DirectX::XMLoadFloat3(&nodeAnim.ScalingKeys[left].Value),
                DirectX::XMLoadFloat3(&nodeAnim.ScalingKeys[right].Value), f);
        }
        // 3. 回転 (Rotation)
        DirectX::XMVECTOR rot;
        if (nodeAnim.RotaionKeys.empty())
        {
            rot = defaultRot;
        }
        else if (nodeAnim.RotaionKeys.size() == 1)
        {
            rot = DirectX::XMLoadFloat4(&nodeAnim.RotaionKeys[0].Value);
        }
        else
        {
            size_t left = 0, right = 0; float f = 0.0f;
            CalcInterpolationFactor(timeInTicks, nodeAnim.RotaionKeys, left, right, f);
            rot = DirectX::XMQuaternionSlerp(
                DirectX::XMLoadFloat4(&nodeAnim.RotaionKeys[left].Value),
                DirectX::XMLoadFloat4(&nodeAnim.RotaionKeys[right].Value), f);
        }
        return { scale, rot, pos };
    }
}
