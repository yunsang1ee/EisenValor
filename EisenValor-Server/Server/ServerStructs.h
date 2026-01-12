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