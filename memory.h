#pragma once

#include "types.h"

#define GAME_HEAP_SIZE (128u << 20)

void initMemory(byte* baseAddress, size_t size);
void resetTransient();

memoryArena_t* create_arena(size_t size);
void release_arena(memoryArena_t* arena);

const char* str_alloc(const char* source);
void* trans_alloc(size_t size);
void* alloc(memoryArena_t** arena, size_t size);


