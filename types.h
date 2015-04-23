#pragma once

struct v2i {
	int x, y;
};

struct file_t
{
	int size;
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
	int hp;
	int attack;
	int defense;
	int damage;
};

struct player_t {
	v2i position;

	int hp;
	int attack;
	int defense;
	int damage;

	monster_t* target;
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
	player_t player;
	level_t currentLevel;
};