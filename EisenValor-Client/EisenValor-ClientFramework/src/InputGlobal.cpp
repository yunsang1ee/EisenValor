#include "stdafxClientFramework.h"
#include "InputGlobal.h"

void InputGlobal::Initialize(HWND hwnd)
{
	m_hwnd = hwnd;
	std::memset(m_InputState, 0, sizeof(m_InputState));
	m_InputEventsFront.clear();
	m_InputEventsBack.clear();

	if (m_hwnd)
	{
		RECT clientRect;
		if (GetClientRect(m_hwnd, &clientRect))
		{
			m_centerX = (clientRect.right - clientRect.left) / 2;
			m_centerY = (clientRect.bottom - clientRect.top) / 2;
			DEBUG_LOG_FMT("[InputGlobal] Initialized - Center: ({}, {})\n", m_centerX, m_centerY);
		}
	}
}

void InputGlobal::BeforeUpdate()
{
	std::swap(m_InputEventsFront, m_InputEventsBack);
	m_InputEventsBack.clear();
	m_consumedState.reset();
	for (const auto& [keyIndex, isPressed, isUp] : m_InputEventsFront)
	{
		uint8_t mask = 0;
		if (not isPressed && not isUp)
			mask = InputBits::Down | InputBits::Pressed;
		else if (isPressed && not isUp)
			mask = InputBits::Pressed;
		else if (isUp)
			mask = InputBits::Up;

		m_InputState[keyIndex] = (m_InputState[keyIndex] & ~(InputBits::Down | InputBits::Up)) | mask;

		//DEBUG_LOG_FMT(
		//	"{:#X}\t{:#} | {:}\n", keyIndex, keyIndex,
		//	(m_InputState[keyIndex] & InputBits::Down)		? "Down"
		//	: (m_InputState[keyIndex] & InputBits::Up)		? "Up"
		//	: (m_InputState[keyIndex] & InputBits::Pressed) ? "Pressed"
		//													: ""
		//);
	}
}

void InputGlobal::AfterUpdate()
{
	for (const auto& [keyIndex, isPressed, isUp] : m_InputEventsFront)
	{
		if (isUp)
			m_InputState[keyIndex] &= ~(InputBits::Pressed | InputBits::Up);
		else if (GetInputDown(keyIndex))
			m_InputState[keyIndex] &= ~InputBits::Down;
	}
	m_MouseState.wheelDelta = 0;
	m_MouseState.deltaX = 0;
	m_MouseState.deltaY = 0;

	UpdateMouseLock();
}

void InputGlobal::SetMouseLocked(bool locked)
{
	if (m_mouseLocked == locked)
	{
		return;
	}

	m_mouseLocked = locked;

	if (locked)
	{
		ShowCursor(FALSE);
		UpdateMouseLock();
		DEBUG_LOG_FMT("[InputGlobal] Mouse locked to center: ({}, {})\n", m_centerX, m_centerY);
	}
	else
	{
		ShowCursor(TRUE);
		DEBUG_LOG_FMT("[InputGlobal] Mouse unlocked\n");
	}
}

void InputGlobal::SetWindowFocused(bool focused)
{
	m_isWindowFocused = focused;
}

void InputGlobal::OnResize(uint32_t width, uint32_t height)
{
	m_centerX = static_cast<int>(width / 2);
	m_centerY = static_cast<int>(height / 2);

	DEBUG_LOG_FMT("[InputGlobal] OnResize - New center: ({}, {})\n", m_centerX, m_centerY);

	if (m_mouseLocked && m_isWindowFocused && m_hwnd)
	{
		POINT centerPoint = {m_centerX, m_centerY};
		ClientToScreen(m_hwnd, &centerPoint);
		SetCursorPos(centerPoint.x, centerPoint.y);
	}
}

void InputGlobal::UpdateMouseLock()
{
	if (!m_mouseLocked || !m_hwnd || !m_isWindowFocused)
	{
		return;
	}

	POINT centerPoint = {m_centerX, m_centerY};
	ClientToScreen(m_hwnd, &centerPoint);
	SetCursorPos(centerPoint.x, centerPoint.y);
}