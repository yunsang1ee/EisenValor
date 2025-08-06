#pragma once

struct KinematicInfo {
	Vec3 position;
	Vec3 rotation;
	Vec3 velocity;
};

struct GeneralInfo {
	uint32		id;
	KinematicInfo	transform;
};

struct SoldierInfo {
	uint32		id;
	KinematicInfo	transform;
};