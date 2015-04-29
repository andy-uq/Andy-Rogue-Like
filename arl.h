#pragma once
#include <functional>

#define internal static
#define NULL 0

#include "types.h"

collection_t* createCollection(size_t initialSize, size_t itemSize);
void add(collection_t* collection, void* item);
void* newItem(collection_t* collection, size_t sizeofItem);
void* getAt(collection_t* collection, uint index);
int count(collection_t* collection);

#define foreach(as, i, collection) auto collectionIterator = collection->head; \
								   for(auto i = (as)collectionIterator->item; collectionIterator; collectionIterator = collectionIterator->next, i = collectionIterator ? (as )collectionIterator->item : 0)

void init_genrand(unsigned long s);
double genrand_real1();

/* strings */
char* str_append(char* dest, const char* source);
char* str_trimend(char* str);
void str_trimstart(char** str);
void str_trim(char** str);
bool str_startswith(const char* target, const char* compareTo);
bool str_endswith(const char* target, const char* compareTo);

bool clamp(v2i* pos, int maxX, int maxY);

/* game */
void initGame();
void initGame(gameState_t* game);
void updateAndRender();
void processInput(const game_input input);
void saveGame(gameState_t* game);
void loadGame(gameState_t* game);
void loadMonster(file_t* file, monster_t* monster);
void loadMonsters(file_t* file, collection_t* monsters);
void loadItems(file_t* file, collection_t* items);

/* map */
bool isDoor(mapElement_t* e);
elementType_t getMapElement(level_t* level, int x, int y);
elementType_t getMapElement(level_t* level, v2i pos);

/* files */
bool parseKey(char* buffer, const char* key, char** value);
int nextInt(const char** value);
char* parseValue(char* line);
int parseKeyValue(char* line, std::function<int(const char*, const char*)> func);

