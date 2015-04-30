#pragma once

typedef unsigned char byte;
typedef unsigned int uint;

typedef unsigned char boolean;
#define true ((boolean )1)
#define false ((boolean )0)

typedef struct {
	int x, y;
} v2i;

typedef struct memoryArena_t memoryArena_t;
typedef struct collectionNode_t collectionNode_t;

struct collectionNode_t
{
	void* item;
	collectionNode_t *next;
};

typedef struct
{
	collectionNode_t* head;
	memoryArena_t* itemStorage;
	memoryArena_t* storage;
	int capacity;
} collection_t;

typedef struct
{
	int size;
} file_t;

typedef struct {
	short xOffset, yOffset;
	boolean quit;
} game_input;

typedef enum elementType_t {
	FLOOR,
	WALL,
	DOOR,
	OPEN_DOOR,
	END_OF_MAP
} elementType_t;

typedef struct {
	elementType_t type;
} mapElement_t;

typedef struct {
	char glyph;
	const char* name;
} item_t;

typedef struct 
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
} monster_t;

typedef struct {
	v2i position;

	int hp;
	int attack;
	int defense;
	int damage;

	monster_t* target;
} player_t;

typedef struct 
{
	memoryArena_t* storage;

	const char* filename;
	mapElement_t* map;
	v2i size;

	collection_t* mobs;
	item_t* items;
} level_t;

typedef struct
{
	player_t player;
	level_t currentLevel;

	collection_t* items;
} gameState_t;
