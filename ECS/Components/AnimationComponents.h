#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <DirectXMath.h>

#include "AnimStateType.h"
#include "AnimationClip.h"
#include "AnimSequenceAsset.h"
#include "SkeletonTypes.h"

namespace Engine
{
    struct SkeletonComponent
    {
        std::vector<Bone> bones;
        int rootBoneIndex = -1;
    };

    struct AnimationComponent
    {
        std::shared_ptr<AnimationClip> pCurrentClip = nullptr;
        float currentTime   = 0.0f;
        float playbackSpeed = 1.0f;
        bool isLoop         = false;
        bool isPlaying      = false;
    };

    struct AnimationState
    {
        AnimStateType animStateType = AnimStateType::NONE;
        std::shared_ptr<AnimSequenceAsset> pAnimSeq = nullptr;
        bool isLoop = false;
    };

    struct AnimatorComponent
    {
        std::unordered_map<AnimStateType, AnimationState> stateMap;
        AnimStateType currentState = AnimStateType::NONE;
        AnimStateType prevState = AnimStateType::NONE;

        std::shared_ptr<AnimationClip> pPrevClip = nullptr;
        float prevTime = 0.0f;
        float transitionDuration = 0.0f;
        float transitionTimer = 0.0f;
        float transitionWeight = 0.0f;
        bool isTransitioning = false;

        void AddState(const AnimStateType animStateType, const std::shared_ptr<AnimSequenceAsset>& pAnimSeq, bool isLoop)
        {
            AnimationState animState;
            animState.animStateType = animStateType;
            animState.pAnimSeq = pAnimSeq;
            animState.isLoop = isLoop;

            auto [it, inserted] = stateMap.try_emplace(animStateType, std::move(animState));
            assert(inserted);
        }
    };

    struct JointConstraint
    {
        bool isLimited = false;
        DirectX::XMVECTOR localAxis = DirectX::XMVectorZero();
        float minAngleRad = 0.0f;
        float maxAngleRad = 0.0f;
    };

    enum class IKSolverType
    {
        FABRIK,
        CCD
    };

    struct IKComponent
    {
        IKSolverType                             solverType = IKSolverType::FABRIK;
        int                                      effectorBoneIndex = -1;
        std::vector<int>                         chainIndices;
        std::unordered_map<int, JointConstraint> constraints;
        DirectX::XMFLOAT3                        targetPosition{ 0, 0, 0 };
        int                                      maxIterations = 10;
        float                                    tolerance = 0.001f;
        float                                    testTimer = 0.0f;
        bool                                     isEnabled = false;

        void SetConstraint(int boneIndex, DirectX::XMVECTOR localAxis, float minAngleDeg, float maxAngleDeg)
        {
            JointConstraint constraint;
            constraint.isLimited = true;
            constraint.localAxis = DirectX::XMVector3Normalize(localAxis);
            constraint.minAngleRad = DirectX::XMConvertToRadians(minAngleDeg);
            constraint.maxAngleRad = DirectX::XMConvertToRadians(maxAngleDeg);
            constraints[boneIndex] = constraint;
        }
    };

    struct AnimationUpdateTag {};
}
