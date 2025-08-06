#pragma once
#include "IInputGlobal.h"

class InputGlobal : public GlobalMakerBase<InputGlobal, IInputGlobal>
{
	struct DInputEvent
	{
		InputCode code;
		bool	  isPressed;
		bool	  isUp;
	};

protected:
	InputGlobal()
	{
		m_InputEventsFront.reserve(DInputState::kMaxInputCode);
		m_InputEventsBack.reserve(DInputState::kMaxInputCode);
	}

public:
	void Initialize() final;
	void BeforeUpdate() final;
	void AfterUpdate() final;

#pragma region OnEvent
	inline void OnInputState(InputCode code, bool isPressed, bool isUp) noexcept final
	{
		m_InputEventsBack.emplace_back(code, isPressed, isUp);
	}

	inline void OnMouseMove(int x, int y) noexcept final
	{
		m_MouseState.deltaX = static_cast<float>(x - m_MouseState.x);
		m_MouseState.deltaY = static_cast<float>(y - m_MouseState.y);
		m_MouseState.x = x;
		m_MouseState.y = y;
	}

	inline void OnWheelScroll(int delta) noexcept final { m_MouseState.wheelDelta = static_cast<float>(delta); }
#pragma endregion

#pragma region Getter
	[[nodiscard]] inline bool GetInputDown(InputCode code) const noexcept final
	{
		return (m_InputState[code] & DInputBits::Down) != 0;
	}

	[[nodiscard]] inline bool GetInput(InputCode code) const noexcept final
	{
		return (m_InputState[code] & DInputBits::Pressed) != 0;
	}

	[[nodiscard]] inline bool GetInputUp(InputCode code) const noexcept final
	{
		return (m_InputState[code] & DInputBits::Up) != 0;
	}

	[[nodiscard]] inline int GetWheelScroll() const noexcept final { return static_cast<int>(m_MouseState.wheelDelta); }

	[[nodiscard]] inline DX::XMFLOAT2 GetMousePosition() const noexcept final
	{
		return {float(m_MouseState.x), float(m_MouseState.y)};
	}
#pragma endregion

private:
	DInputState m_InputState{};
	DMouseState m_MouseState{};

	std::vector<DInputEvent> m_InputEventsFront;
	std::vector<DInputEvent> m_InputEventsBack;
};