#pragma once

#include <stddef.h>

#define internal static

#include "types.h"

collection_t* createCollection(size_t initialSize, size_t itemSize);
void add(collection_t* collection, void* item);
void* newItem(collection_t* collection, size_t sizeofItem);
void* getAt(collection_t* collection, uint index);
int count(collection_t* collection);

#define foreach(as, iterator, collection) collectionNode_t* __collectionIterator = collection->head; \
								   for(as iterator = (as)__collectionIterator->item; __collectionIterator; __collectionIterator = __collectionIterator->next, iterator = __collectionIterator ? (as )__collectionIterator->item : (as )0)

void init_genrand(unsigned long s);
double genrand_real1();

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
void initGame();
void initGameStruct(gameState_t* game);
void updateAndRender();
void processInput(const game_input input);
void saveGame(gameState_t* game);
void loadGame(gameState_t* game);
void loadMonster(file_t* file, monster_t* monster);
void loadMonsters(file_t* file, collection_t* monsters);
void loadItems(file_t* file, collection_t* items);

/* map */
boolean isDoor(mapElement_t* e);
elementType_t getMapElement(level_t* level, int x, int y);

/* files */
boolean tryGetValueIfKey(char* buffer, const char* key, char** value);
void parseKeyValue(char* line, char** key, char** value);

