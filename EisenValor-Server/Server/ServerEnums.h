#pragma once

enum class COMPONENT_TYPE : uint8 {
	FSM,
	BEHAVIOR_TREE,
	TROOP_CONTROLLER,

	END
};

enum GENERAL_STATE_TYPE : uint16 {
	NONE = 0,
	IDLE,
	MOVE,
	PRE_DELAY,
	ATTACK,
	POST_DELAY,
	DEFENSE,
	STUN,
	DEAD,

	END
};

enum class GENERAL_SUB_STATE_TYPE : uint8 {
	NONE = 0,
	EXHAUSTED = 1 << 0,
};

inline constexpr GENERAL_SUB_STATE_TYPE operator|(GENERAL_SUB_STATE_TYPE a, GENERAL_SUB_STATE_TYPE b) noexcept
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) | static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator|=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b) noexcept
{
	a = a | b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator&(GENERAL_SUB_STATE_TYPE a, GENERAL_SUB_STATE_TYPE b) noexcept
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) & static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator&=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b) noexcept
{
	a = a & b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator^(GENERAL_SUB_STATE_TYPE a, GENERAL_SUB_STATE_TYPE b) noexcept
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(static_cast<uint8>(a) ^ static_cast<uint8>(b));
}

inline constexpr GENERAL_SUB_STATE_TYPE& operator^=(GENERAL_SUB_STATE_TYPE& a, GENERAL_SUB_STATE_TYPE b) noexcept
{
	a = a ^ b;
	return a;
}

inline constexpr GENERAL_SUB_STATE_TYPE operator~(GENERAL_SUB_STATE_TYPE a) noexcept
{
	return static_cast<GENERAL_SUB_STATE_TYPE>(~static_cast<uint8>(a));
}