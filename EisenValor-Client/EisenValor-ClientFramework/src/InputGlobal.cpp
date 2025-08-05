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
			mask = DInputBits::Down | DInputBits::Pressed;
		else if (isPressed && not isUp)
			mask = DInputBits::Pressed;
		else if (isUp)
			mask = DInputBits::Up;

		m_InputState[keyIndex] = (m_InputState[keyIndex] & ~(DInputBits::Down | DInputBits::Up)) | mask;

		DEBUG_LOG_FMT(
			"{:#X}\t{:#} | {:}\n", keyIndex, keyIndex,
			(m_InputState[keyIndex] & DInputBits::Down)		 ? "Down"
			: (m_InputState[keyIndex] & DInputBits::Up)		 ? "Up"
			: (m_InputState[keyIndex] & DInputBits::Pressed) ? "Pressed"
															 : ""
		);
	}
}

void InputGlobal::AfterUpdate()
{
	for (const auto& [keyIndex, isPressed, isUp] : m_InputEventsFront)
	{
		if (isUp)
			m_InputState[keyIndex] &= ~(DInputBits::Pressed | DInputBits::Up);
		else if (GetInputDown(keyIndex))
			m_InputState[keyIndex] &= ~DInputBits::Down;
	}
	m_MouseState.wheelDelta = 0;
	m_MouseState.deltaX = 0;
	m_MouseState.deltaY = 0;
}