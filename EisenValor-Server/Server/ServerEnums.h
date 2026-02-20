#pragma once

enum class COMPONENT_TYPE : uint8 {
	FSM,
	BEHAVIOR_TREE,
	NAV_AGENT,
	COLLIDER,

	END
};

enum class COLLIDER_TYPE : uint8 { NONE, SPHERE, AABB, OBB, END };

enum class GENERAL_SUB_STATE_TYPE : uint8 {
	NONE = 0,
	EXHAUSTED = 1 << 0,
};