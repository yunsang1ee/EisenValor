#pragma once
#include "Singleton.h"
#include <vector>
#include <bitset>

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
	void Initialize(HWND hwnd);
	void BeforeUpdate();
	void AfterUpdate();

	void SetMouseLocked(bool locked);
	bool IsMouseLocked() const { return m_mouseLocked; }

	void SetWindowFocused(bool focused);

	void OnResize(uint32_t width, uint32_t height);

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
		if (m_mouseLocked && m_centerX > 0 && m_centerY > 0)
		{
			m_MouseState.deltaX = static_cast<float>(x - m_centerX);
			m_MouseState.deltaY = static_cast<float>(y - m_centerY);
			m_MouseState.x = m_centerX;
			m_MouseState.y = m_centerY;
		}
		else
		{
			m_MouseState.deltaX = static_cast<float>(x - m_MouseState.x);
			m_MouseState.deltaY = static_cast<float>(y - m_MouseState.y);
			m_MouseState.x = x;
			m_MouseState.y = y;
		}
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

	[[nodiscard]] inline DX::XMFLOAT2 GetMouseDelta() const noexcept
	{
		return {m_MouseState.deltaX, m_MouseState.deltaY};
	}

	// 소비 상태 마킹 및 확인
	[[nodiscard]] inline bool IsConsumed(InputCode code) const noexcept
	{
		if (code >= kMaxInputCode)
		{
			return false;
		}
		return m_consumedState[code];
	}

	inline void SetConsumed(InputCode code) noexcept
	{
		if (code >= kMaxInputCode)
		{
			DEBUG_LOG_FMT("[InputGlobal] Invalid InputCode in SetConsumed: {}\n", code);
			return;
		}
		m_consumedState[code] = true;
	}

	[[nodiscard]] inline bool GetUnConsumedInput(InputCode code) const noexcept
	{
		return GetInput(code) && !IsConsumed(code);
	}

	[[nodiscard]] inline bool GetUnConsumedInputDown(InputCode code) const noexcept
	{
		return GetInputDown(code) && !IsConsumed(code);
	}

	[[nodiscard]] inline bool GetUnConsumedInputUp(InputCode code) const noexcept
	{
		return GetInputUp(code) && !IsConsumed(code);
	}

private:
	void UpdateMouseLock();

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

	HWND m_hwnd = nullptr;
	bool m_mouseLocked = false;
	bool m_isWindowFocused = true;
	// 소비 상태 저장 배열
	std::bitset<kMaxInputCode> m_consumedState;
	int	 m_centerX = 0;
	int	 m_centerY = 0;
};