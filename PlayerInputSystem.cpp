#include <DirectXMath.h>

#include "PlayerInputSystem.h"
#include "InputSystem.h"
#include "Components/CharacterComponents.h"
#include "Components/CombatComponents.h"

namespace
{
	using namespace Engine;

	DirectX::XMFLOAT2 CalcMoveDir(InputSystem& input)
	{
		DirectX::XMFLOAT2 moveDirLocal{ 0.0f, 0.0f };
		if (input.IsKeyPressed('W')) moveDirLocal.y += 1.0f;
		if (input.IsKeyPressed('S')) moveDirLocal.y -= 1.0f;
		if (input.IsKeyPressed('A')) moveDirLocal.x -= 1.0f;
		if (input.IsKeyPressed('D')) moveDirLocal.x += 1.0f;

		return moveDirLocal;
	}

	bool CalcActionBufferTimer(PlayerInputComponent& playerInput, float dt)
	{
		if (playerInput.actionBufferTimer > 0.0f)
		{
			playerInput.actionBufferTimer -= dt;
			if (playerInput.actionBufferTimer <= 0.0f)
			{
				return true;
			}
		}

		return false;
	}

	void TryRegistBufferAction(PlayerInputComponent& playerInput, CombatInputIntent nextAction, float bufferTime)
	{
		if (static_cast<uint8_t>(nextAction) >= static_cast<uint8_t>(playerInput.bufferAction))
		{
			playerInput.bufferAction = nextAction;
			playerInput.actionBufferTimer = bufferTime;
		}
	}

	void DecideCurrentCombatAction(InputSystem& inputSystem, PlayerInputComponent& playerInput)
	{
		if (inputSystem.IsKeyTriggered(VK_LBUTTON))
		{
			TryRegistBufferAction(playerInput, CombatInputIntent::COMBO, 0.2f); // 0.2秒間有効
		}
		if (inputSystem.IsKeyTriggered(VK_SPACE))
		{
			TryRegistBufferAction(playerInput, CombatInputIntent::GUARD, 0.25f); // 0.25秒間有効
		}
	}
}

namespace Engine
{
	void PlayerInputSystem::Update(entt::registry& registry, float dt)
	{
		// 
		auto inputSystem = InputSystem::GetInstance();

		// 
		auto view = registry.view<PlayerTag, PlayerInputComponent, CombatRequestComponent>();
		auto playerEntity = view.front();
		auto& playerInput = registry.get<PlayerInputComponent>(playerEntity);
		auto& combatReq = registry.get<CombatRequestComponent>(playerEntity);

		// 移動系の入力計算
		DirectX::XMFLOAT2 moveAxis = CalcMoveDir(inputSystem);
		bool isSprinting = inputSystem.IsKeyPressed(VK_SHIFT);
		// 移動系の入力を登録
		playerInput.moveAxis = moveAxis;
		playerInput.isSprintPressed = isSprinting;

		// アクション系の入力バッファタイマーの計算
		bool isFinidhedActionBufferTimer = CalcActionBufferTimer(playerInput, dt);
		if (isFinidhedActionBufferTimer)
		{
			playerInput.bufferAction = CombatInputIntent::NONE;
			combatReq.isExecuted = false;
		}
		// アクション系の入力を登録
		DecideCurrentCombatAction(inputSystem, playerInput);
	}
}