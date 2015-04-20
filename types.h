#pragma once

struct v2i {
	int x, y;
};

struct game_input {
	short xOffset, yOffset;
	bool quit;
};

enum elementType_t {
	FLOOR,
	WALL,
	DOOR,
	OPEN_DOOR,


	END_OF_MAP
};

struct mapElement_t {
	elementType_t type;
};

struct monster_t
{
	char glyph;
	v2i position;
	v2i target;
	int speed;
	int energy;
};

struct level_t
{
	const char* filename;
	mapElement_t* map;
	v2i size;

	monster_t* mobs;
};

struct gameState_t
{
	v2i charPos;
	level_t currentLevel;
};