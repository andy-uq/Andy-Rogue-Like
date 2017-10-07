#include <assert.h>
#include <string.h>

#include "arl.h"
#include "memory.h"

#define HASH_MAX_COLLISIONS 10

struct hashtable_t
{
	uint(*hash)(void* key, int keySize);
	boolean(*match)(void* key, void* compareTo, int keySize);
	arena_t* storage;
	byte* keys;
	void** items;
	int capacity;
	int key_size;
	int count;
};

arena_t* hashtables = NULL;
arena_t* collections = NULL;

collection_t* transient_collection(size_t initialSize, size_t itemSize)
{
	collection_t* collection = (collection_t*)transient_alloc(sizeof(collection_t));
	collection->head = 0;
	collection->count = 0;
	collection->capacity = initialSize > 10 ? initialSize : 10;
	collection->item_storage = transient_arena(collection->capacity * itemSize);
	collection->node_storage = transient_arena(collection->capacity * sizeof(collection_node_t));

	return collection;
}

collection_t* collection_from_arena(arena_t** arena)
{
	collection_t* collection = (collection_t*)arena_alloc(arena, sizeof(collection_t));
	collection->head = 0;
	collection->count = 0;
	collection->node_storage = *arena;
	collection->item_storage = *arena;

	return collection;
}

collection_t* create_collection(size_t initialSize, size_t itemSize)
{
	collection_t* collection = (collection_t*)arena_alloc(&collections, sizeof(collection_t));
	collection->head = 0;
	collection->count = 0;
	collection->capacity = initialSize > 10 ? initialSize : 10;
	collection->node_storage = arena_create(collection->capacity * sizeof(collection_node_t));

	collection->item_storage = itemSize 
		? arena_create(initialSize * itemSize)
		: 0;

	return collection;
}

internal
collection_node_t* _alloc_node(collection_t* collection)
{
	if (collection->free_list == 0)
	{
		return arena_alloc(&collection->node_storage, sizeof(collection_node_t));
	}

	collection_node_t* node = collection->free_list;
	collection->free_list = node->next;
	return node;
}

internal
void _remove_node(collection_t* collection, collection_node_t* node)
{
	node->next = collection->free_list;
	collection->free_list = node;
}

void collection_push(collection_t* collection, void* item)
{
	collection_node_t* node = _alloc_node(collection);
	node->item = item;
	node->next = collection->head;
	collection->head = node;
	collection->count++;
}

void* collection_new_item(collection_t* collection, size_t sizeofItem)
{
	assert(collection->item_storage);

	void* item = arena_alloc(&collection->item_storage, sizeofItem);
	collection_push(collection, item);
	return item;
}

void* collection_first(collection_t* collection)
{
	collection_node_t* p = collection ? collection->head : 0;
	return p ? p->item : p;
}

void* collection_single(collection_t* collection)
{
	collection_node_t* p = collection ? collection->head : 0;
	return p && !p->next ? p->item : 0;
}

void* collection_pop(collection_t* collection)
{
	if (collection && collection->head)
	{
		collection_node_t* head = collection->head;
		collection->head = head->next;
		collection->count--;

		_remove_node(collection, head);		
		return head->item;
	}

	return 0;
}

void collection_remove(collection_t* collection, void* item)
{
	if (collection && collection->head)
	{
		if (collection->head->item == item)
		{
			collection_pop(collection);
			return;
		}

		collection_node_t *p = collection->head;
		while (p->next)
		{
			if (p->next->item == item)
			{
				collection_node_t* node = p->next;
				p->next = node->next;

				collection->count--;
				_remove_node(collection, node);
				return;
			}

			p = p->next;
		}
	}
}

boolean collection_any(collection_t* collection)
{
	collection_node_t* p = collection ? collection->head : 0;
	return p != 0;
}

void* collection_get_at(collection_t* collection, uint index)
{
	collection_node_t* p = collection ? collection->head : 0;
	while (p)
	{
		if (index == 0)
			break;

		index--;
		p = p->next;
	}

	return p ? p->item : 0;
}

void* collection_next(collection_t* collection, void* item)
{
	collection_node_t* p = collection ? collection->head : 0;
	while (p)
	{
		if (p->item == item)
		{
			p = p->next;
			return p ? p->item : 0;
		}

		p = p->next;
	}

	return p ? p->item : 0;
}

void* collection_prev(collection_t* collection, void* item)
{
	collection_node_t* p = collection ? collection->head : 0;
	void* tail = 0;
	while (p)
	{
		if (p->item == item)
		{
			return tail;
		}

		tail = p->item;
		p = p->next;
	}

	return 0;
}

int collection_count(collection_t* collection)
{
	return collection->count;
}

internal
uint _key_hash_binary(void* key, int keySize)
{
	const uint Prime = 0x1000193;
	uint hash = 0x811c9dc5;
	byte* data = key;

	while (keySize)
	{
		hash = (hash * Prime) ^ *data;
		data++;
		keySize--;
	}

	return hash;
}

internal
boolean _key_match_binary(void* key, void* compareTo, int keySize)
{
	return memcmp(key, compareTo, keySize) == 0;
}

hashtable_t* create_hashtable(int capacity, int keySize)
{
	hashtable_t* hashtable = arena_alloc(&hashtables, sizeof(hashtable_t));

	size_t size = capacity*(sizeof(void*) + keySize);
	hashtable->hash = _key_hash_binary;
	hashtable->match = _key_match_binary;
	hashtable->key_size = keySize;
	hashtable->storage = arena_create(size);
	hashtable->items = arena_alloc(&hashtable->storage, capacity * sizeof(void*));
	hashtable->keys = arena_alloc(&hashtable->storage, capacity * (sizeof(keySize) + 1));
	hashtable->capacity = capacity;
	hashtable->count = 0;

	return hashtable;
}

hashtable_t* hashtable_from_arena(arena_t* arena, int capacity, int keySize)
{
	hashtable_t* hashtable = arena_alloc(&hashtables, sizeof(hashtable_t));

	hashtable->hash = _key_hash_binary;
	hashtable->match = _key_match_binary;
	hashtable->key_size = keySize;
	hashtable->items = arena_alloc(&arena, capacity * sizeof(void*));
	hashtable->keys = arena_alloc(&arena, capacity *(sizeof(keySize) + 1));
	hashtable->capacity = capacity;
	hashtable->storage = 0;
	hashtable->count = 0;

	return hashtable;
}

#define HASHKEY(keys, bucketid, keySize) ((keys) + (bucketid * (keySize+1)))

internal
boolean _inner_hashtable_add(void** itemStore, byte* keyStore, int capacity, uint hash, int keySize, void* key, void* item)
{
	uint start = hash % capacity;
	uint bucketId = start;
	
	byte* targetKey = HASHKEY(keyStore, bucketId, keySize);
	while (*targetKey)
	{
		bucketId = (bucketId + 1) % capacity;
		if (bucketId == start)
			return false;

		targetKey = HASHKEY(keyStore, bucketId, keySize);
	}

	*targetKey = 1;
	itemStore[bucketId] = item;
	memcpy(targetKey + 1, key, keySize);

	return true;
}

boolean hashtable_resize(hashtable_t* hashtable, int capacity)
{
	if (hashtable->storage == 0)
		return false;

	size_t size = capacity*(sizeof(void*) + hashtable->key_size + 1);
	arena_t* storage = arena_create(size);
	void** itemStore = arena_alloc(&storage, capacity*sizeof(void*));
	byte* keyStore = arena_alloc(&storage, capacity*(hashtable->key_size + 1));

	for (int b = 0; b < hashtable->capacity; b++)
	{
		byte* key = HASHKEY(hashtable->keys, b, hashtable->key_size);
		if (*key)
		{
			uint hash = hashtable->hash(key + 1, hashtable->key_size);
			if (!_inner_hashtable_add(itemStore, keyStore, capacity, hash, hashtable->key_size, key + 1, hashtable->items[b]))
			{
				arena_destroy(storage);
				return false;
			}
		}
	}

	arena_destroy(hashtable->storage);

	hashtable->storage = storage;
	hashtable->keys = keyStore;
	hashtable->items = itemStore;
	hashtable->capacity = capacity;
	return true;
}

boolean hashtable_contains(hashtable_t* hashtable, void* key)
{
	uint hash = hashtable->hash(key, hashtable->key_size);
	uint start = hash % hashtable->capacity;
	uint bucketId = start;
	byte* target = HASHKEY(hashtable->keys, bucketId, hashtable->key_size);

	while (*target)
	{
		if (hashtable->match(target + 1, key, hashtable->key_size))
			return true;

		bucketId = (bucketId + 1) % hashtable->capacity;
		if (bucketId == start)
			break;

		target = HASHKEY(hashtable->keys, bucketId, hashtable->key_size);
	}

	return false;
}

void* hashtable_get(hashtable_t* hashtable, void* key)
{
	uint hash = hashtable->hash(key, hashtable->key_size);
	uint start = hash % hashtable->capacity;
	uint bucketId = start;
	byte* target = HASHKEY(hashtable->keys, bucketId, hashtable->key_size);

	while (*target)
	{
		if (hashtable->match(target + 1, key, hashtable->key_size))
			return hashtable->items[bucketId];

		bucketId = (bucketId + 1) % hashtable->capacity;
		if (bucketId == start)
			break;

		target = HASHKEY(hashtable->keys, bucketId, hashtable->key_size);
	}

	return 0;
}

boolean hashtable_add(hashtable_t* hashtable, void* key, void* item)
{
	if (hashtable_contains(hashtable, key))
		return false;

	uint hash = hashtable->hash(key, hashtable->key_size);
	if (_inner_hashtable_add(hashtable->items, hashtable->keys, hashtable->capacity, hash, hashtable->key_size, key, item))
	{
		hashtable->count++;
		return true;
	}

	return false;
}

boolean hashtable_remove(hashtable_t* hashtable, void* key)
{
	uint hash = hashtable->hash(key, hashtable->key_size);
	uint start = hash % hashtable->capacity;
	uint bucketId = start;
	byte* target = HASHKEY(hashtable->keys, bucketId, hashtable->key_size);

	while (*target)
	{
		if (hashtable->match(target + 1, key, hashtable->key_size))
		{
			*target = 0;
			hashtable->items[bucketId] = 0;
			hashtable->count--;

			return true;
		}

		bucketId = (bucketId + 1) % hashtable->capacity;
		if (bucketId == start)
			break;

		target = HASHKEY(hashtable->keys, bucketId, hashtable->key_size);
	}

	return false;
}

void hashtable_clear(hashtable_t* hashtable)
{
	hashtable->count = 0;
	memset(hashtable->keys, 0, (sizeof(hashtable->key_size) + 1) * hashtable->capacity);
	memset(hashtable->items, 0, sizeof(void*) * hashtable->capacity);
}

void hashtable_destroy(hashtable_t* hashtable)
{
	arena_destroy(hashtable->storage);
}

hashtable_t* create_int_hashtable(int capacity)
{
	return create_hashtable(capacity, sizeof(int));
}
