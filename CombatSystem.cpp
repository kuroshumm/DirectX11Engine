#include "CombatSystem.h"
#include "InputSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"

namespace
{
	using namespace Engine;

	/// <summary>
	/// 攻撃の入力から次に再生するアクションを計算
	/// </summary>
	/// <param name="combatInputIntent"></param>
	/// <returns></returns>
	CombatAttackID CalcNextCombatAttackID(
		CombatInputIntent& combatInputIntent,
		CombatAttackID activeAttackID
	)
	{
		// コンボ以外は受け付けない
		if (combatInputIntent != CombatInputIntent::COMBO)
		{
			return CombatAttackID::NONE;
		}

		switch (activeAttackID)
		{
		case Engine::CombatAttackID::COMBO_1:
			return CombatAttackID::COMBO_2;
		case Engine::CombatAttackID::COMBO_2:
			break;
		default:
			return CombatAttackID::COMBO_1;
		}
	}
}

namespace Engine
{
	void CombatSystem::Update(entt::registry& registry)
	{
		auto view = registry.view<
			PlayerTag,
			PlayerInputComponent,
			CharacterCombatStateComponent,
			CombatRequestComponent
		>();

		for (auto&& [entity, playerInput, combatState, combatReq] : view.each())
		{
			// アクションの設定
			combatReq.combatAttackID = CalcNextCombatAttackID(
				playerInput.bufferAction,
				combatState.activeAttackId
			);

			// 攻撃対象のターゲット計算
			// TODO：周囲の状況や入力から適切なターゲットを計算
			combatReq.targetEntity = entt::null;
		}
	}
}
