#include <assert.h>

#include "arl.h"
#include "memory.h"

#define INITIAL_ARENA_COUNT 100

#define PERMANENT_STORE_SIZE (32 << 20)
#define STRING_STORE_SIZE (32 << 20)


memoryArena_t* freeList;
memoryArena_t* arenaStore;

typedef struct 
{
	byte* head;
	byte* tail;
	size_t size;
} block_t;

struct memoryArena_t
{
	byte* head;
	byte* tail;
	size_t size;
	memoryArena_t* next;
};

block_t permanentStore;
block_t transientStore;
block_t stringStore;

internal 
void initBlock(block_t* block, byte* baseAddress, size_t size)
{
	block->head = baseAddress;
	block->tail = baseAddress + size;
	block->size = size;
}

internal
void* raw_alloc(block_t* block, size_t size)
{
	assert (size < (size_t)(block->tail - block->head) /* "BLOCK FULL" */);

	byte* memory = block->head;
	block->head += size;

	return memory;
}

void initMemory(byte* baseAddress, size_t size)
{
	assert(size > PERMANENT_STORE_SIZE + STRING_STORE_SIZE /* "SIZE NOT BIG ENOUGH FOR PERMANENT_STORE_SIZE AND STRING_STORE_SIZE" */);

	initBlock(&permanentStore, baseAddress, PERMANENT_STORE_SIZE);
	initBlock(&stringStore, permanentStore.tail, STRING_STORE_SIZE);
	initBlock(&transientStore, stringStore.tail, size - (PERMANENT_STORE_SIZE + STRING_STORE_SIZE));

	assert(transientStore.tail == baseAddress + size /* TAIL MUST BE END OF ALLOCATED RANGE */);

	size_t arenaStoreSize = sizeof(memoryArena_t) * INITIAL_ARENA_COUNT;
	arenaStore = (memoryArena_t*)raw_alloc(&permanentStore, arenaStoreSize);
	arenaStore->size = arenaStoreSize;
	arenaStore->head = (byte* )&arenaStore[1];
	arenaStore->tail = arenaStore->head + arenaStore->size;
}

void resetTransient()
{
	transientStore.head = transientStore.tail - transientStore.size;
}

void* trans_alloc(size_t size)
{
	return raw_alloc(&transientStore, size);
}

const char* str_alloc(const char* string)
{
	char* result = (char* )stringStore.head;
	byte* p = stringStore.head;

	while (*string)
	{
		(*p++) = *string++;
		assert(p < stringStore.tail /* DONT OVERWRITE NEXT ALLOCATION BLOCK */);
	}

	*p = 0;
	stringStore.head = p + 1;

	return result;
}

memoryArena_t* create_arena(size_t size)
{
	if (freeList) {
		memoryArena_t* best = 0;
		for (memoryArena_t* p = freeList; p; p = p->next) {
			if (p->size > size)
			{
				if (!best || best->size > p->size) {
					best = p;
				}
			}
		}

		if (best)
			return best;
	}

	memoryArena_t* arena = (memoryArena_t*)alloc(&arenaStore, sizeof(memoryArena_t));
	arena->size = size;
	arena->head = (byte*)raw_alloc(&permanentStore, size);
	arena->tail = arena->head + arenaStore->size;

	return arena;
}

internal
int max(int a, int b) 
{ 
	return a < b ? a : b; 
}

void* alloc(memoryArena_t** pArena, size_t size)
{
	memoryArena_t* arena = *pArena;
	if (arena == 0 || (size_t)(arena->tail - arena->head) < size)
	{
		int newSize = arena ? max(size, arena->size) : size;
		*pArena = (memoryArena_t *)create_arena(newSize);
		(*pArena)->next = arena;
		arena = *pArena;
	}

	byte* alloc = arena->head;
	arena->head += size;
	return alloc;
}

void release_arena(memoryArena_t* arena) {
	if (freeList) 
	{
		memoryArena_t* p = freeList;
		while (p->next)
			p++;

		p->next = arena;
	}
	else {
		freeList = arena;
	}

	arena->head = (arena->tail - arena->size);
}