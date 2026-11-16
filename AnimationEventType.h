#pragma once
#include <cstdint>

namespace Engine
{
    // ---------------------------------------------------
    // 区間イベント
    // ---------------------------------------------------
    enum class AnimEventWindowType : uint8_t
    {
        NONE,

        COMBO_QUEUE,          // 次段コンボ予約受付
        ATTACK_COLLISION,     // 攻撃コリジョン

        COUNT
    };

    struct TimeRange
    {
        float start = 0.0f;
        float end = 0.0f;
    };

    struct AnimWindowEvent
    {
        AnimEventWindowType type;
        TimeRange range;
    };

    // ---------------------------------------------------
    // 単体イベント
    // ---------------------------------------------------
    enum class AnimPointType : uint8_t
    {
        NONE,

        COUNT
    };

    struct AnimPointEvent
    {
        AnimPointType type = AnimPointType::NONE;
        float triggerTime = 0.0f;
    };
}
