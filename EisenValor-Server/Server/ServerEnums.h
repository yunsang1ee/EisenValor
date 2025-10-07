#pragma once

enum class GAME_OBJECT_TYPE : uint8 {
	PLAYER,
	NPC,
	VALLISTAR,
	PROJECTILE,

	END
};

enum class NPC_TYPE : uint8 {
	NONE,
	GENERAL,
	SOLDIER,

	END
};

enum class TEAM_TYPE : uint8 {
	BLUE,
	RED,

	COUNT
};

enum class SOLDIER_FORMATION : uint8 {
	FORMATION_1,
	FORMATION_2,
	FORMATION_3,

	END
};

enum class COMPONENT_TYPE : uint8 {
	FSM,
	BEHAVIOR_TREE,
	TROOP_CONTROLLER,

	END
};