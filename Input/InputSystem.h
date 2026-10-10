#pragma once
#include <windows.h>
#include "Vector2.h"

namespace Engine
{
	class InputSystem
	{
	public:
		InputSystem();
		~InputSystem();

		// どこからでもアクセスできるようにするためのシングルトンインスタンスの取得
		static InputSystem& GetInstance()
		{
			static InputSystem instance;
			return instance;
		}

		// キーの状態を更新する関数
		void UpdateKey(WPARAM wParam, bool isPressed);
		void PostUpdateKey();

		// マウスの状態を更新する関数 
		void UpdateMouse(LPARAM lParam);
		// マウスの変化量を初期化する関数
		void ResetMouseDelta();

		// キーの状態を取得する関数
		bool IsKeyPressed(WPARAM key) const;
		// キーを押した瞬間か
		bool IsKeyTriggered(WPARAM key) const;
		// マウスの移動量を取得
		bool TryGetMouseDelta(Vector2& mouseDelta) const;

	private:
		bool m_keys[256];		// キーの状態を格納する配列
		bool m_prevKeys[256];	// 前回フレームのキー状態

		Vector2 m_mouse; // マウスのXY座標
		Vector2 m_lastMouse; // 前フレームのXY座標
		Vector2 m_mouseDelta; // 前フレームのXY座標の変化量
	};
}