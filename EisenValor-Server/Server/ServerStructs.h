#pragma once

struct KinematicInfo {
	Vec3 position;
	Vec3 rotation;
	Vec3 velocity;
	Vec3 acceleration;
	uint64 timeStamp;
};

struct StatInfo {
	uint32 hp;	// Ã¼·Â
};