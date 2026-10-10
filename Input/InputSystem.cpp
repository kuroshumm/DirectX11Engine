#include "InputSystem.h"
#include <stdio.h>
#include <algorithm>

namespace Engine
{
	InputSystem::InputSystem() 
	{
		std::fill(std::begin(m_keys), std::end(m_keys), false);

		m_mouse = Vector2::zero;
		m_lastMouse = Vector2::minus;
		m_mouseDelta = Vector2::zero;
	}
	InputSystem::~InputSystem() {}

	void InputSystem::UpdateKey(WPARAM key, bool isPressed)
	{
		// WPARAMは負になることはないので上限チェックのみ
		if (key > 255)
		{
			return;
		}

		// WPARAMはキーコードを表すため、対応するインデックスに状態を保存
		m_keys[key] = isPressed;
	}

	void InputSystem::PostUpdateKey()
	{
		std::copy(std::begin(m_keys), std::end(m_keys), std::begin(m_prevKeys));
		ResetMouseDelta();
	}

	void InputSystem::UpdateMouse(LPARAM lParam)
	{
		m_mouse.x = static_cast<float>(LOWORD(lParam));
		m_mouse.y = static_cast<float>(HIWORD(lParam));

		if (m_lastMouse.x < 0.0f)
		{
			m_lastMouse = m_mouse;
		}

		m_mouseDelta += (m_mouse - m_lastMouse);

		m_lastMouse = m_mouse;
	}

	void InputSystem::ResetMouseDelta()
	{
		m_mouseDelta = Vector2::zero;
	}

	bool InputSystem::IsKeyPressed(WPARAM key) const
	{
		// WPARAMは負になることはないので上限チェックのみ
		if (key > 255)
		{
			return false;
		}

		return m_keys[key];
	}

	bool InputSystem::IsKeyTriggered(WPARAM key) const
	{
		// WPARAMは負になることはないので上限チェックのみ
		if (key > 255)
		{
			return false;
		}

		return m_keys[key] && !m_prevKeys[key];
	}

	bool InputSystem::TryGetMouseDelta(Vector2& mouseDelta) const
	{
		if (m_mouseDelta == Vector2::zero)
		{
			mouseDelta = Vector2::zero;
			return false;
		}

		mouseDelta = m_mouseDelta;
		return true;
	}
}