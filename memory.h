#pragma once

#include "types.h"

#define GAME_HEAP_SIZE (128u << 20)

void memory_init(byte* baseAddress, size_t size);
void transient_reset();

arena_t* arena_create(size_t size);
void arena_destroy(arena_t* arena);

const char* string_alloc(const char* source);
void* transient_alloc(size_t size);
void* arena_alloc(arena_t** arena, size_t size);


