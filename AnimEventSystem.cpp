#include "AnimEventSystem.h"

namespace
{
    using namespace Engine;

    // 現在有効か
    bool IsRangeActive(const TimeRange& range, float currentTime)
    {
        return (currentTime >= range.start && currentTime <= range.end);
    }
    // 現在フレームで有効になったか
    bool IsRangeEntered(const TimeRange& range, float prevTime, float currentTime)
    {
        return (prevTime < range.start && currentTime >= range.start);
    }
    // 現在フレームで無効になったか
    bool IsRangeExited(const TimeRange& range, float prevTime, float currentTime)
    {
        return (prevTime <= range.end && currentTime > range.end);
    }
    // 現在フレームで単発イベント時刻を通過したか
    bool IsPointTriggered(float triggerTime, float prevTime, float currentTime)
    {
        return (prevTime < triggerTime && currentTime >= triggerTime);
    }

    
    void PrepareEventStates(
        AnimEventComponent& animEvent,
        AnimStateType currentAnimState
    )
    {
        // 直前ステート用は1フレームしか使わないため、毎フレーム先頭で空にする
        animEvent.prev = AnimEventState{};

        // 現在ステート用の単発フラグ(Enter / Exit)をクリア
        animEvent.current.isEnteredWindow.fill(false);
        animEvent.current.isExitedWindow.fill(false);

        // 同じステートが継続しているならそのまま
        if (animEvent.current.animState == currentAnimState)
        {
            return;
        }

        // ステートが切り替わった場合：
        // 旧ステート(current)で Active だったものを prev の Exit に移す
        animEvent.prev.animState = animEvent.current.animState;
        for (size_t i = 0; i < AnimEventState::WINDOW_MAX; ++i)
        {
            if (animEvent.current.isActiveWindow[i])
            {
                animEvent.prev.isExitedWindow[i] = true;
            }
        }

        // 現在ステート用を新しいステートで初期化
        animEvent.current = AnimEventState{};
        animEvent.current.animState = currentAnimState;

    }

    // 4. 現在再生中のアニメーションのイベントを時間で評価する
    void EvaluateCurrentAnimEvents(
        const AnimatorComponent& animator,
        const AnimationComponent& animation,
        AnimEventState& currentEventState
    )
    {
        const AnimSequenceAsset* currentAnimSeq = animator.GetCurrentAnimSeq();
        if (!currentAnimSeq || !animation.isPlaying || animation.pCurrentClip != currentAnimSeq->clip)
        {
            return;
        }

        currentEventState.isActiveWindow.fill(false);

        const float currentTime = animation.currentTime;
        const float prevTime = animation.prevTime;
        for (const auto& windowEvent : currentAnimSeq->windowEvents)
        {
            const auto idx = static_cast<size_t>(windowEvent.type);
            if (IsRangeActive(windowEvent.range, currentTime))
            {
                currentEventState.isActiveWindow[idx] = true;
            }
            if (IsRangeEntered(windowEvent.range, prevTime, currentTime))
            {
                currentEventState.isEnteredWindow[idx] = true;
            }
            if (IsRangeExited(windowEvent.range, prevTime, currentTime))
            {
                currentEventState.isExitedWindow[idx] = true;
            }
        }
    }
}

namespace Engine
{
    void AnimEventSystem::Update(entt::registry& registry)
    {
        auto view = registry.view<
            AnimatorComponent, 
            AnimationComponent,
            AnimEventComponent
        >();

        for (auto[entity, animator, animation, animEvent] : view.each())
        {
            PrepareEventStates(animEvent, animator.currentState);
            EvaluateCurrentAnimEvents(animator, animation, animEvent.current);
        }

    }
}
