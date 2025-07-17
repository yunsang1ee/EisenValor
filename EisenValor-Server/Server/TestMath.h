#pragma once

struct Vec2 {
	float x, y;
};

struct Vec3 {
	float x, y, z;
};

struct Axis {
	float pitch, yaw, roll;
};

struct Transform{
	Vec3 pos;
	Axis axis;
};