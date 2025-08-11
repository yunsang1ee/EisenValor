#pragma once

struct KinematicInfo {
	Vec3 position;
	Vec3 rotation;
	Vec3 velocity;
	Vec3 acceleration;
	uint64 timeStamp;
};

struct GeneralInfo {
	uint32		id;
	KinematicInfo	transform;
};

struct SoldierInfo {
	uint32		id;
	KinematicInfo	transform;
};