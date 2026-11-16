#pragma once
#include "AnimStateType.h"
#include "CollisionType.h"
#include <DirectXMath.h>
#include <array>
#include <cstdint>

namespace Engine
{
    enum class CombatInputIntent : uint8_t
    {
        NONE,
        COMBO,
        STRIKE,
        COUNTER,
        GUARD,
    };

    enum class CombatAttackID : uint8_t
    {
        NONE,
        COMBO_1,
        COMBO_2,
        CLOSE_STRIKE,
        LEAP_STRIKE,
        COUNTER_ATTACK,
        GUARD,

        COUNT
    };

    enum class ActionState : uint8_t
    {
        LOCOMOTION,
        ATTACK,
        DODGE,
        DAMAGED,
        DEAD
    };

    struct CombatAttackDesc
    {
        AnimStateType animState = AnimStateType::NONE;
        float fadeDuration = 0.1f;
        DirectX::XMFLOAT3 collisionOffset{ 0.0f, 1.0f, 0.8f };
        CollisionShapeDesc collisionShape = SphereShapeDesc{ 0.5f };
        int damage = 0;
    };

    struct CombatAttackPreset
    {
        std::array<CombatAttackDesc, static_cast<size_t>(CombatAttackID::COUNT)> attacks;
    };

    namespace CombatPresets
    {
        inline const CombatAttackPreset HumanoidDefault =
        {
            {
                // NONE
                CombatAttackDesc{},
                // COMBO_1
                CombatAttackDesc{
                    AnimStateType::COMBO1,
                    0.1f,
                    DirectX::XMFLOAT3(0.1f, 1.3f, -0.6f),
                    BoxShapeDesc{ DirectX::XMFLOAT3(0.25f, 0.25f, 0.35f) },
                    1
                },
                // COMBO_2
                CombatAttackDesc{
                    AnimStateType::COMBO2,
                    0.15f,
                    DirectX::XMFLOAT3(-0.1f, 1.3f, -0.6f),
                    BoxShapeDesc{ DirectX::XMFLOAT3(0.25f, 0.25f, 0.35f) },
                    1
                },
                // CLOSE_STRIKE 〜 GUARD
                CombatAttackDesc{},
                CombatAttackDesc{},
                CombatAttackDesc{},
                CombatAttackDesc{}
            }
        };
    }
} // namespace Engine
