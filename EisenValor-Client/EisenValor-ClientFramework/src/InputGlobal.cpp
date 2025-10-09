#include "stdafxClientFramework.h"
#include "InputGlobal.h"

void InputGlobal::Initialize() {}

void InputGlobal::BeforeUpdate()
{
	std::swap(m_InputEventsFront, m_InputEventsBack);
	m_InputEventsBack.clear();
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

		DEBUG_LOG_FMT(
			"{:#X}\t{:#} | {:}\n", keyIndex, keyIndex,
			(m_InputState[keyIndex] & InputBits::Down)		? "Down"
			: (m_InputState[keyIndex] & InputBits::Up)		? "Up"
			: (m_InputState[keyIndex] & InputBits::Pressed) ? "Pressed"
															: ""
		);
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
}