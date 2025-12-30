#pragma once

struct KinematicInfo {
	Vec3 position;
	Vec3 rotation;
	Vec3 velocity;
	Vec3 acceleration;
	uint64 timeStamp;
};

struct StatInfo {
	uint32 hp;			// 체력
	uint32 atk;			// 공격력
	uint32 stamina;		// 스태미나
};

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