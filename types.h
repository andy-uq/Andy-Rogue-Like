#pragma once

typedef unsigned char byte;
typedef unsigned int uint;

typedef unsigned char boolean;
#define true ((boolean )1)
#define false ((boolean )0)

typedef struct {
	int x, y;
} v2i;

typedef struct arena_t arena_t;
typedef struct collection_node_t collection_node_t;

struct collection_node_t
{
	void* item;
	collection_node_t *next;
};

typedef struct
{
	collection_node_t* head;
	arena_t* item_storage;
	arena_t* node_storage;
	int capacity;
	int count;
} collection_t;

typedef struct
{
	int size;
} file_t;

typedef struct {
	short x_offset, y_offset;
	boolean quit;
} game_input_t;

typedef enum {
	FLOOR,
	WALL,
	DOOR,
	OPEN_DOOR,
	END_OF_MAP
} element_type_t;

typedef struct {
	element_type_t type;
	collection_t* items;
} map_element_t;

typedef struct {
	int id;
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
	
	collection_t* inventory;

	monster_t* target;
} player_t;

typedef struct 
{
	arena_t* storage;

	const char* filename;
	map_element_t* map;
	v2i size;

	collection_t* mobs;
} level_t;

typedef struct
{
	player_t player;
	level_t current_level;

	collection_t* items;
} game_t;
