#include <assert.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

#define INITIAL_ARENA_COUNT 100

#define PERMANENT_STORE_SIZE (32 << 20)
#define STRING_STORE_SIZE (32 << 20)

memoryArena_t* freeList;
memoryArena_t* arenaStore;

typedef struct 
{
	const char* name;
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

block_t permanentStore = { "Permanent store" };
block_t transientStore = { "Transient store" };
block_t stringStore = { "String store" };

internal 
void initBlock(block_t* block, byte* baseAddress, size_t size)
{
	block->head = baseAddress;
	block->tail = baseAddress + size;
	block->size = size;

	debugf("created block %s (0x%p) of %d bytes", block->name, block->head, size);
}

internal
void* raw_alloc(block_t* block, size_t size)
{
	assert (size < (size_t)(block->tail - block->head) /* "BLOCK FULL" */);

	byte* memory = block->head;
	block->head += size;
	debugf("allocated %d bytes (0x%p) from %s (%d bytes remaining)", size, memory, block->name, (size_t)(block->tail - block->head));

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
	byte* head = transientStore.tail - transientStore.size;
	if (transientStore.head == head)
		return;

	debugf("reset transient (%d was used)", transientStore.head - head);
	transientStore.head = head;
}

void* trans_alloc(size_t size)
{
	return raw_alloc(&transientStore, size);
}

const char* str_alloc(const char* string)
{
	char* result = (char* )stringStore.head;
	byte* p = stringStore.head;
	int length = 0;
	while (*string)
	{
		(*p++) = *string++;
		length++;

		assert(p < stringStore.tail /* DONT OVERWRITE NEXT ALLOCATION BLOCK */);
	}

	*p = 0;
	stringStore.head = p + 1;
	debugf("new string in heap (%d bytes) %d bytes remaining", length + 1, (size_t)(stringStore.tail - stringStore.head));

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
		{
			debugf("recycled arena of %d bytes (0x%p) (we needed %d which has wasted %d bytes)", best->size, best->head, size, best->size - size);
			return best;
		}
	}

	memoryArena_t* arena = (memoryArena_t*)alloc(&arenaStore, sizeof(memoryArena_t));
	arena->size = size;
	arena->head = (byte*)raw_alloc(&permanentStore, size);
	arena->tail = arena->head + arenaStore->size;
	debugf("create new arena of %d bytes (0x%p)", arena->size, arena->head);

	return arena;
}

internal
int max(int a, int b) 
{ 
	return a < b ? a : b; 
}

internal
void* alloc_arena(memoryArena_t* arena, size_t size)
{
	byte* alloc = arena->head;
	arena->head += size;
	return alloc;
}

void* alloc(memoryArena_t** pArena, size_t size)
{
	memoryArena_t* arena = *pArena;
	if (arena)
	{
		size_t available = arena->tail - arena->head;
		if (available < size)
		{
			int newSize = max(size, arena->size);
			debugf("chaining new arena of %d bytes because we don't have enough room in this one", newSize);

			*pArena = (memoryArena_t *)create_arena(newSize);
			(*pArena)->next = arena;
			return alloc_arena(*pArena, size);
		}

		return alloc_arena(*pArena, size);
	}
	else 
	{
		*pArena = (memoryArena_t *)create_arena(size);
		return alloc_arena(*pArena, size);
	}
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
	debugf("released arena of %d bytes (0x%p) back into pool", arena->size, arena->head);
}