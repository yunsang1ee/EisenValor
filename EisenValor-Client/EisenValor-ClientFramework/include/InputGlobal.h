#pragma once
#include "Singleton.h"
#include <vector>

inline constexpr size_t kMaxInputCode = 256;

namespace InputBits
{
inline constexpr uint8_t None = 0x00;
inline constexpr uint8_t Down = 0x01;
inline constexpr uint8_t Pressed = 0x02;
inline constexpr uint8_t Up = 0x04;
} // namespace InputBits

struct DMouseState
{
	int	  x = 0;
	int	  y = 0;
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	float wheelDelta = 0.0f;
};

using InputCode = uint8_t;

class InputGlobal : public Singleton<InputGlobal>
{
private:
	friend class Singleton<InputGlobal>;

	InputGlobal()
	{
		m_InputEventsFront.reserve(kMaxInputCode);
		m_InputEventsBack.reserve(kMaxInputCode);
	}

	~InputGlobal() override = default;

public:
	void Initialize();
	void BeforeUpdate();
	void AfterUpdate();

	inline void OnInputState(InputCode code, bool isPressed, bool isUp) noexcept
	{
		if (code >= kMaxInputCode)
		{
			DEBUG_LOG_FMT("Invalid InputCode: {}\n", code);
			return;
		}
		m_InputEventsBack.emplace_back(code, isPressed, isUp);
	}

	inline void OnMouseMove(int x, int y) noexcept
	{
		m_MouseState.deltaX = static_cast<float>(x - m_MouseState.x);
		m_MouseState.deltaY = static_cast<float>(y - m_MouseState.y);
		m_MouseState.x = x;
		m_MouseState.y = y;
	}

	inline void OnWheelScroll(int delta) noexcept { m_MouseState.wheelDelta = static_cast<float>(delta); }

	[[nodiscard]] inline bool GetInputDown(InputCode code) const noexcept
	{
		return (m_InputState[code] & InputBits::Down) != 0;
	}

	[[nodiscard]] inline bool GetInput(InputCode code) const noexcept
	{
		return (m_InputState[code] & InputBits::Pressed) != 0;
	}

	[[nodiscard]] inline bool GetInputUp(InputCode code) const noexcept
	{
		return (m_InputState[code] & InputBits::Up) != 0;
	}

	[[nodiscard]] inline int GetWheelScroll() const noexcept { return static_cast<int>(m_MouseState.wheelDelta); }

	[[nodiscard]] inline DX::XMFLOAT2 GetMousePosition() const noexcept
	{
		return {float(m_MouseState.x), float(m_MouseState.y)};
	}

private:
	struct DInputEvent
	{
		InputCode code;
		bool	  isPressed;
		bool	  isUp;
	};

	uint8_t					 m_InputState[kMaxInputCode] = {};
	DMouseState				 m_MouseState{};
	std::vector<DInputEvent> m_InputEventsFront;
	std::vector<DInputEvent> m_InputEventsBack;
};