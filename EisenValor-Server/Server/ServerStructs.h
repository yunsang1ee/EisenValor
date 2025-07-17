#pragma once

struct Attribute {
	int level;
	int hp;
	int atk;
};

struct Identity {
	GAME_OBJECT_TYPE	type;
	uint32				id;
	std::wstring		name;
};