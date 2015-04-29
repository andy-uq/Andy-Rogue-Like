#pragma once

typedef unsigned char byte;
typedef unsigned int uint;

struct v2i {
	int x, y;
};

struct memoryArena_t
{
	byte* head;
	byte* tail;
	size_t size;
	memoryArena_t* next;
};

struct collectionNode_t
{
	void* item;
	collectionNode_t *next;
};

struct collection_t
{
	collectionNode_t* head;
	memoryArena_t* itemStorage;
	memoryArena_t* storage;
	int capacity;
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

struct item_t {
	char glyph;
	char* name;
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

	collection_t* mobs;
	item_t* items;
};

struct gameState_t
{
	player_t player;
	level_t currentLevel;

	item_t* items;
};
