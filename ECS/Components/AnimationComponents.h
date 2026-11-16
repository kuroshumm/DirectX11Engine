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
        float prevTime      = 0.0f;
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

        AnimSequenceAsset* GetCurrentAnimSeq() const
        {
            if (currentState == AnimStateType::NONE)
            {
                return nullptr;
            }

            auto it = stateMap.find(currentState);
            if (it == stateMap.end() || !it->second.pAnimSeq)
            {
                return nullptr;
            }

            return it->second.pAnimSeq.get();
        }

    };

    struct AnimEventState
    {
        static constexpr size_t WINDOW_MAX = static_cast<size_t>(AnimEventWindowType::COUNT);
        static constexpr size_t POINT_MAX = static_cast<size_t>(AnimPointType::COUNT);

        std::array<bool, WINDOW_MAX> isActiveWindow{};
        std::array<bool, WINDOW_MAX> isEnteredWindow{};
        std::array<bool, WINDOW_MAX> isExitedWindow{};
        std::array<bool, WINDOW_MAX> isTriggeredPoint{};

        // 現在評価しているステート
        AnimStateType animState = AnimStateType::NONE;
    };
    struct AnimEventComponent
    {
        AnimEventState current;
        AnimEventState prev;
    };
    inline bool IsEventWindowActive(const AnimEventComponent& animEvent, AnimStateType state, AnimEventWindowType type)
    {
        const size_t idx = static_cast<size_t>(type);
        
        if (animEvent.current.animState == state)
        {
            return animEvent.current.isActiveWindow[idx];
        }
        if (animEvent.prev.animState == state)
        {
            return animEvent.prev.isActiveWindow[idx];
        }

        return false;
    }
    inline bool IsEventWindowEntered(const AnimEventComponent& animEvent, AnimStateType state, AnimEventWindowType type)
    {
        const size_t idx = static_cast<size_t>(type);

        if (animEvent.current.animState == state)
        {
            return animEvent.current.isEnteredWindow[idx];
        }
        if (animEvent.prev.animState == state)
        {
            return animEvent.prev.isEnteredWindow[idx];
        }

        return false;
    }
    inline bool IsEventWindowExited(const AnimEventComponent& animEvent, AnimStateType state, AnimEventWindowType type)
    {
        const size_t idx = static_cast<size_t>(type);

        if (animEvent.current.animState == state)
        {
            return animEvent.current.isExitedWindow[idx];
        }
        if (animEvent.prev.animState == state)
        {
            return animEvent.prev.isExitedWindow[idx];
        }

        return false;
    }

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
