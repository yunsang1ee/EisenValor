#pragma once

struct RoomInfo {
	uint16 id;
	FB_ENUMS::ROOM_STATE_TYPE stateType;
	uint8 currentParticipants;
	uint8 maxParticipants;
};

struct ParticipantInfo {
	uint32								id;
	FB_ENUMS::PARTICIPANT_TYPE			type;
	FB_ENUMS::PARTICIPANT_STATE_TYPE	stateType;
	FB_ENUMS::TEAM_TYPE					teamType;
};

struct PosInfo {
	Vec3 pos;
	Vec3 rot;
};

struct Stat {
	uint32 currentHP;
	uint32 maxHP;
	uint32 currentStamina;
	uint32 maxStamina;
	uint32 respawnTimeSec;
};

struct SkillData;

struct AttackInfo {
	const SkillData*					skillData;
	FB_ENUMS::GENERAL_ATTACK_DIR_TYPE	dir;
	uint64								startPreDelay;
	uint64								startPostDelay;
};
