#pragma once

#include <stddef.h>

#define internal static

#include "types.h"

collection_t* create_collection(size_t initialSize, size_t itemSize);
void collection_add(collection_t* collection, void* item);
void* collection_new_item(collection_t* collection, size_t sizeofItem);
void* collection_get_at(collection_t* collection, uint index);
int collection_count(collection_t* collection);

#define foreach(as, iterator, collection) collection_node_t* __collectionIterator = collection->head; \
								   for(as iterator = (__collectionIterator ? (as )__collectionIterator->item : 0) \
										; __collectionIterator \
										; __collectionIterator = __collectionIterator->next, iterator = (__collectionIterator ? (as )__collectionIterator->item : 0))

void rand_init(unsigned long s);
double rand_r();

/* strings */
char* str_append(char* dest, const char* source);
char* str_trimend(char* str);
void str_trimstart(char** str);
void str_trim(char** str);
boolean str_startswith(const char* target, const char* compareTo);
boolean str_endswith(const char* target, const char* compareTo);
boolean str_equals(const char* target, const char* compareTo);

boolean clamp(v2i* pos, int maxX, int maxY);

/* game */
void init_game();
void init_game_struct(game_t* game);
void update_and_render();
void process_input(const game_input_t input);
void save_game(game_t* game);
void load_game(game_t* game);
void load_monster(file_t* file, monster_t* monster);
void load_monsters(file_t* file, collection_t* monsters);

void load_items(file_t* file, collection_t* items);
void set_item_property(const char* key, const char* value, item_t* item);

/* map */
boolean is_door(map_element_t* e);
element_type_t get_map_element(level_t* level, int x, int y);

/* files */
boolean parse_value_if_match(char* buffer, const char* key, char** value);
void parse_key_value(char* line, char** key, char** value);
