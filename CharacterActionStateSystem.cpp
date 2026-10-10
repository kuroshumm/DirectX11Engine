#include "CharacterActionStateSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"

namespace Engine
{
	void CharacterActionStateSystem::Update(entt::registry& registry, float dt)
	{
		auto view = registry.view<
			PlayerTag,
			LocomotionProposalComponent,
			CombatRequestComponent,
			CharacterActionStateComponent,
			CharacterCombatStateComponent
		>();

		for (auto&& [entity, locomotionProposal, combatReq, charaActionState, charaCombatState] : view.each())
		{
			// ステートの滞在時間更新
			charaActionState.stateTimer += dt;

			// 攻撃要求があるか
			bool hasCombatRequest = !combatReq.isExecuted 
									&& combatReq.combatAttackID != CombatAttackID::NONE;

			switch (charaActionState.currentState)
			{
			case ActionState::LOCOMOTION:
				// 移動中に攻撃要求がきたら攻撃状態へ遷移
				if (hasCombatRequest)
				{
					// 攻撃ステートに遷移
					charaActionState.currentState = ActionState::ATTACK;
					charaActionState.stateTimer = 0.0f;

					// 攻撃サブ状態を初期化
					charaCombatState.activeAttackId = combatReq.combatAttackID;
					charaCombatState.attackTimer = 0.0f;
					charaCombatState.isHitRegistered = false;

					combatReq.isExecuted = true;
				}
				break;
			case ActionState::ATTACK:
				// 攻撃実行システム側で技が終了した場合、通常移動へ復帰
				if (charaCombatState.activeAttackId == CombatAttackID::NONE)
				{
					charaActionState.currentState = ActionState::LOCOMOTION;
					charaActionState.stateTimer = 0.0f;
				}
				// TODO: コンボ先行入力受付ウィンドウ内なら、次段攻撃へキャンセル遷移
				break;
			case ActionState::DODGE:
				break;
			case ActionState::HIT_STUN:
				break;
			}

		}
	}
}