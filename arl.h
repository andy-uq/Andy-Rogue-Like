#pragma once

#include <stddef.h>

#define internal static

#include "types.h"

void hashtable_destroy(hashtable_t* hashtable);
void hashtable_clear(hashtable_t* hashtable);
void* hashtable_get(hashtable_t* hashtable, void* key);
boolean hashtable_remove(hashtable_t* hashtable, void* key);
boolean hashtable_add(hashtable_t* hashtable, void* key, void* item);
boolean hashtable_contains(hashtable_t* hashtable, void* key);
boolean hashtable_resize(hashtable_t* hashtable, int capacity);
hashtable_t* create_hashtable(int capacity, int keySize);
hashtable_t* create_int_hashtable(int capacity);
hashtable_t* hashtable_from_arena(arena_t* arena, int capacity, int keySize);

collection_t* transient_collection(size_t initialSize, size_t itemSize);
collection_t* collection_from_arena(arena_t** arena);
collection_t* create_collection(size_t initialSize, size_t itemSize);
void collection_push(collection_t* collection, void* item);
void* collection_pop(collection_t* collection);
void collection_remove(collection_t* collection, void* item);
void* collection_new_item(collection_t* collection, size_t sizeofItem);
void* collection_first(collection_t* collection);
boolean collection_any(collection_t* collection);
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

void load_items(file_t* file, arena_t* storage, hashtable_t* items);
item_t* find_item(collection_t* items, int id);
void set_item_property(const char* key, const char* value, item_t* item);
void set_player_item_property(const char* key, const char* value, stacked_item_t* item);

stacked_item_t* stacked_find(collection_t* collection, int itemid);
stacked_item_t* stacked_add(collection_t* collection, item_t* item);
item_t* stacked_remove(collection_t* collection, stacked_item_t* stacked);

/* map */
boolean is_door(map_element_t* e);
map_element_t* get_map_element(level_t* level, int x, int y);
map_element_t* get_player_tile(level_t* level, player_t* player);
element_type_t get_map_element_type(level_t* level, int x, int y);
void drop_item(level_t* level, item_t* item, int x, int y);
item_t* pickup_item(level_t* level, int x, int y);
collection_t* items_on_floor(level_t* level, int x, int y);

/* files */
boolean parse_value_if_match(char* buffer, const char* key, char** value);
void parse_key_value(char* line, char** key, char** value);
