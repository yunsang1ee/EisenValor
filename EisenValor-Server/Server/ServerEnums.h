#pragma once

enum class GAME_OBJECT_TYPE : uint8 {
	PLAYER,
	NPC,
	SPAWN_BASE,	// Creature,	별도	Component 장착
	PROJECTILE, // GameObject,	별도	Component 제작

	END
};

enum class NPC_TYPE : uint8 {
	NONE, 
	GENERAL,
	SOLDIER,
	ARCHER,
	MEDIC,
	BATTLE_RAM,
	BOSS,

	END
};

enum class TEAM_TYPE : uint8 {
	ALLY,
	ENEMY,

	END
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