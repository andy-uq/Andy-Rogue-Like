#pragma once
#include <functional>

#define internal static
#define NULL 0

#include "types.h"

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
void loadMonsters(file_t* file, monster_t* monster);

/* map */
bool isDoor(mapElement_t* e);
elementType_t getMapElement(level_t* level, int x, int y);
elementType_t getMapElement(level_t* level, v2i pos);

/* files */
bool parseKey(char* buffer, const char* key, char** value);
int nextInt(const char** value);
char* parseValue(char* line);
int parseKeyValue(char* line, std::function<int(const char*, const char*)> func);

