#include "AttackingStateSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"
#include "Components/AnimationComponents.h"

namespace Engine
{
	void AttackingStateSystem::Update(entt::registry& registry, float dt)
	{
		auto view = registry.view<
			PlayerTag,
			CharacterActionStateComponent,
			CharacterCombatStateComponent,
			CharacterActionResultComponent
		>();

		for (auto&& [entity, charaActionState, combatState, charaActionResult] : view.each())
		{
			// 戦闘以外では何もしない
			if (charaActionState.currentState != ActionState::ATTACK)
			{
				continue;
			}

			// 1. 技ID (CombatAttackID) に応じたアニメーション名とパラメータの決定
			AnimStateType animName = AnimStateType::NONE;
			float duration = 0.8f;
			switch (combatState.activeAttackId)
			{
			case CombatAttackID::COMBO_1:
				animName = AnimStateType::COMBO1;
				duration = 0.8f;
				break;
			case CombatAttackID::COMBO_2:
				animName = AnimStateType::COMBO2;
				duration = 0.7f;
				break;
			default:
				break;
			}
			
			charaActionResult.targetAnim = animName;
			charaActionResult.currentFadeDuration = duration;

			// 攻撃中は移動しない
			// TODO：ターゲットがいる場合に移動する
			charaActionResult.velocity = { 0.0f, 0.0f, 0.0f };
			
			// TODO：ターゲットの方向を向く
			charaActionResult.targetYaw = 0.0f;

			// 攻撃中は旋回しない
			charaActionResult.shouldRotate = false;
			charaActionResult.turnSpeed = 0.0f;

			// アニメーション完了したら終了
			combatState.attackTimer += dt;
			if (combatState.attackTimer >= duration)
			{
				combatState.activeAttackId = CombatAttackID::NONE;
			}
		}
	}
}
