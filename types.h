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
typedef struct hashtable_t hashtable_t;
typedef struct stringtable_t stringtable_t;
typedef struct game_t game_t;

struct collection_node_t
{
	void* item;
	collection_node_t *next;
};

typedef struct
{
	collection_node_t* head;
	collection_node_t* free_list;
	arena_t* item_storage;
	arena_t* node_storage;
	int capacity;
	int count;
} collection_t;

typedef struct
{
	int size;
} file_t;

typedef enum
{
	GAME_ACTION_NONE,
	GAME_ACTION_PICKUP,
	GAME_ACTION_DROP,
	GAME_ACTION_QUIT,
	GAME_ACTION_CANCEL,
	GAME_ACTION_OK,
} GAME_ACTION;

typedef struct {
	short x_offset, y_offset;
	GAME_ACTION action;
} game_input_t;

typedef struct message_t {
	char* message;
} message_t;

typedef struct messages_t {
	message_t* list;
	int head;
	int insertIndex;
} messages_t;

typedef struct message_iterator_t
{
	int index;
} message_iterator_t;

typedef enum {
	FLOOR,
	WALL,
	DOOR,
	OPEN_DOOR,
	END_OF_MAP
} element_type_t;

typedef struct {
	element_type_t type;
} map_element_t;

typedef struct {
	int id;
	char glyph;
	const char* name;
} item_t;

typedef struct
{
	item_t* item;
	int x, y;
} map_item_t;

typedef struct
{
	item_t* item;
	int quantity;
} stacked_item_t;

typedef struct {
	boolean active;
	collection_t* (*items)();
	boolean(*confirm)(game_t* game, stacked_item_t* item);
	const char* title;
	stacked_item_t* selected;
} select_item_t;

typedef struct
{
	int min;
	int max;
} int_range_t;

typedef struct
{
	int id;

	char* name;
	char* description;
	
	int_range_t level;
	int_range_t speed;

	int_range_t energy;
	int_range_t hp;
	int_range_t attack;
	int_range_t defense;
	int_range_t damage;
} monster_info_t;

typedef struct
{
	monster_info_t* info;

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
	long currency;

	collection_t* inventory;

	monster_t* target;
} player_t;

typedef struct 
{
	arena_t* storage;

	const char* filename;
	map_element_t* map;
	hashtable_t* items;
	v2i size;

	collection_t* mobs;
} level_t;

typedef struct game_t
{
	player_t player;
	level_t current_level;
	select_item_t select_item; // current dialog being selected from
	messages_t messages;

	arena_t* storage;
	hashtable_t* items;
} game_t;
