#pragma once
#define NOMINMAX
#include <windows.h>
#include <limits>
#include <array>
#include <cassert>
#include <stdexcept>
#include <format>

using InputCode = BYTE;
using InputState = BYTE;

struct DInputBits
{
	static constexpr InputState Down = 0b0001;
	static constexpr InputState Pressed = 0b0010;
	static constexpr InputState Up = 0b0100;
};

struct DInputState
{
	static constexpr size_t				  kMaxInputCode = std::numeric_limits<InputCode>::max() + 1;
	std::array<InputState, kMaxInputCode> inputs{};

	DInputState() = default;

public:
#pragma region Indexing
	[[nodiscard]] constexpr InputState& operator[](InputCode code) noexcept
	{
		assert(code < kMaxInputCode);
		return inputs[(code < kMaxInputCode) ? code : 0];
	}

	[[nodiscard]] constexpr const InputState& operator[](InputCode code) const noexcept
	{
		assert(code < kMaxInputCode);
		return inputs[(code < kMaxInputCode) ? code : 0];
	}

	[[nodiscard]] constexpr InputState& at(InputCode code)
	{
		if (code >= kMaxInputCode) [[unlikely]]
		{
			throw std::out_of_range(std::format("InputStates::at - index out of range: {:}", code));
		}
		return inputs[code];
	}

	[[nodiscard]] constexpr const InputState& at(InputCode code) const
	{
		if (code > kMaxInputCode) [[unlikely]]
		{
			throw std::out_of_range(std::format("InputStates::at - index out of range: {:}", code));
		}
		return inputs[code];
	}
#pragma endregion

	void Clear() noexcept { inputs.fill(0); }
};

struct DMouseState
{
	DMouseState() = default;
	int	  x = 0;
	int	  y = 0;
	float deltaX = 0.0f;
	float deltaY = 0.0f;
	float wheelDelta = 0.0f;

	void Clear() noexcept { deltaX = deltaY = wheelDelta = 0.0f; }
};
