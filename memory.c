#include <assert.h>
#include <string.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

#define INITIAL_ARENA_COUNT 100

#define PERMANENT_STORE_SIZE (32 << 20)
#define STRING_STORE_SIZE (32 << 20)

typedef struct 
{
	const char* name;
	byte* head;
	byte* alloc;
	size_t size;
} block_t;

struct arena_t
{
	byte* head;
	byte* alloc;
	size_t size;
	arena_t* next;
};

internal block_t _gPermanentStore = { "Permanent store" };
internal block_t _gTransientStore = { "Transient store" };
internal block_t _gStringStore = { "String store" };

internal arena_t* freeList;
internal arena_t* arenaStore;

internal 
void init_block(block_t* block, byte* baseAddress, size_t size)
{
	block->head = baseAddress;
	block->alloc = baseAddress;
	block->size = size;

#if _DEBUG
	memset(baseAddress, 0, size);
#endif

	debugf("created block %s (0x%p) of %d bytes", block->name, block->head, size);
}

internal
void* raw_alloc(block_t* block, size_t size)
{
	uint available = block->size - (size_t)(block->alloc - block->head);
	assert (size <= available /* "BLOCK FULL" */);

	byte* memory = block->alloc;
	block->alloc += size;
	debugf("allocated %d bytes (0x%p) from %s (%d bytes remaining)", size, memory, block->name, available);

#if _DEBUG
	memset(memory, 0, size);
#endif

	return memory;
}

void memory_init(byte* baseAddress, size_t size)
{
	assert(size > PERMANENT_STORE_SIZE + STRING_STORE_SIZE /* "SIZE NOT BIG ENOUGH FOR PERMANENT_STORE_SIZE AND STRING_STORE_SIZE" */);

	init_block(&_gPermanentStore, baseAddress, PERMANENT_STORE_SIZE);
	init_block(&_gStringStore, baseAddress + PERMANENT_STORE_SIZE, STRING_STORE_SIZE);
	init_block(&_gTransientStore, baseAddress + PERMANENT_STORE_SIZE + STRING_STORE_SIZE, size - (PERMANENT_STORE_SIZE + STRING_STORE_SIZE));

	assert((_gTransientStore.head + _gTransientStore.size) == baseAddress + size /* TAIL MUST BE END OF ALLOCATED RANGE */);

	size_t arenaStoreSize = sizeof(arena_t) * INITIAL_ARENA_COUNT;
	arenaStore = (arena_t*)raw_alloc(&_gPermanentStore, arenaStoreSize);
	arenaStore->size = arenaStoreSize;
	arenaStore->head = (byte*) arenaStore;
	arenaStore->alloc = arenaStore->head + sizeof(arena_t);
}

void transient_reset()
{
	if (_gTransientStore.alloc == _gTransientStore.head)
		return;

	debugf("reset transient (%d was used)", _gTransientStore.alloc - _gTransientStore.head);
	_gTransientStore.alloc = _gTransientStore.head;

#if _DEBUG
	memset(_gTransientStore.head, 0, _gTransientStore.size);
#endif
}

void* transient_alloc(size_t size)
{
	return raw_alloc(&_gTransientStore, size);
}

boolean is_transient(void* location)
{
	byte* p = (byte*)location;
	return p >= _gTransientStore.head && p < (_gTransientStore.head + _gTransientStore.size);
}

arena_t* transient_arena(size_t size)
{
	arena_t* arena = (arena_t*)transient_alloc(sizeof(arena_t));
	arena->size = size;
	arena->head = (byte*)transient_alloc(size);
	arena->alloc = arena->head;
	debugf("create new transient arena of %d bytes (0x%p)", arena->size, arena->head);

	return arena;
}

const char* string_alloc(const char* string)
{
	char* result = (char* )_gStringStore.head;
	byte* p = _gStringStore.head;
	int length = 0;
	while (*string)
	{
		(*p++) = *string++;
		length++;

		assert(p < _gStringStore.head + _gStringStore.size /* DONT OVERWRITE NEXT ALLOCATION BLOCK */);
	}

	*p = 0;
	_gStringStore.head = p + 1;
	debugf("new string in heap (%d bytes) %d bytes remaining", length + 1, _gStringStore.size - (size_t)(_gStringStore.alloc - _gStringStore.head));

	return result;
}

arena_t* arena_create(size_t size)
{
	if (freeList) {
		arena_t* best = 0;
		for (arena_t* p = freeList; p; p = p->next) {
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

	arena_t* arena = (arena_t*)arena_alloc(&arenaStore, sizeof(arena_t));
	arena->size = size;
	arena->head = (byte*)raw_alloc(&_gPermanentStore, size);
	arena->alloc = arena->head;
	debugf("create new arena of %d bytes (0x%p)", arena->size, arena->head);

	return arena;
}

internal
int max(int a, int b) 
{ 
	return a < b ? a : b; 
}

internal
void* arena_alloc_internal(arena_t* arena, size_t size)
{
	byte* memory = arena->alloc;
	arena->alloc += size;

#if _DEBUG
	memset(memory, 0, size);
#endif
	return memory;
}

void* arena_alloc(arena_t** pArena, size_t size)
{
	arena_t* arena = *pArena;
	if (arena)
	{
		size_t available = arena->size - (arena->alloc - arena->head);
		if (available < size)
		{
			int newSize = max(size, arena->size);

			if (is_transient(pArena))
			{
				*pArena = transient_arena(newSize);
			}
			else
			{
				*pArena = arena_create(newSize);
				debugf("chaining new arena of %d bytes because we don't have enough room in this one", newSize);
			}

			(*pArena)->next = arena;
			return arena_alloc_internal(*pArena, size);
		}

		return arena_alloc_internal(*pArena, size);
	}
	else 
	{
		*pArena = (arena_t *)arena_create(size);
		return arena_alloc_internal(*pArena, size);
	}
}

void arena_destroy(arena_t* arena) {
	if (freeList) 
	{
		arena_t* p = freeList;
		while (p->next)
			p++;

		p->next = arena;
	}
	else {
		freeList = arena;
	}

	arena->alloc = arena->head;
	debugf("released arena of %d bytes (0x%p) back into pool", arena->size, arena->head);
}