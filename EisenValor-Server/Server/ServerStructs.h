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