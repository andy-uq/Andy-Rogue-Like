#include <assert.h>
#include <string.h>

#include "arl.h"
#include "memory.h"

#define HASH_MAX_COLLISIONS 10

typedef struct node_t
{
	void* key;
	void* item;
} node_t;

typedef struct bucket_t
{
	int count;
	node_t nodes[HASH_MAX_COLLISIONS];
} bucket_t;

struct hashtable_t
{
	int(*hash)(void* key);
	boolean(*match)(void* key, void* compareTo);
	arena_t* storage;
	bucket_t* buckets;
	int bucket_count;
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

int collection_count(collection_t* collection)
{
	return collection->count;
}

hashtable_t* create_hashtable(int capacity, int(*hash)(void* key), boolean(*match)(void* key, void* compareTo))
{
	hashtable_t* hashtable = arena_alloc(&hashtables, sizeof(hashtable_t));

	size_t size = capacity*sizeof(bucket_t);
	hashtable->hash = hash;
	hashtable->match = match;
	hashtable->storage = arena_create(size);
	hashtable->buckets = arena_alloc(&hashtable->storage, capacity * sizeof(bucket_t));
	hashtable->bucket_count = capacity;
	hashtable->count = 0;

	return hashtable;
}


internal
int _key_hash_int(void* key)
{
	return *( int* )key;
}

internal
boolean _key_match_int(void* key, void* compareTo)
{
	return *( int* )key == *( int* )compareTo;
}

internal
boolean _inner_hashtable_add(bucket_t* buckets, int bucket_count, int hash, void* key, void* item)
{
	int bucketId = hash % bucket_count;
	bucket_t* bucket = &buckets[bucketId];

	if (bucket->count == HASH_MAX_COLLISIONS)
		return false;

	bucket->nodes[bucket->count++] = (node_t)
	{
		.key = key, .item = item
	};
	return true;
}

boolean hashtable_resize(hashtable_t* hashtable, int capacity)
{
	size_t size = capacity*sizeof(bucket_t);
	arena_t* storage = arena_create(size);
	bucket_t* buckets = arena_alloc(&storage, size);

	for (int b = 0; b < hashtable->bucket_count; b++)
	{
		bucket_t* from_bucket = &hashtable->buckets[b];
		for (int i = 0; i < from_bucket->count; i++)
		{
			node_t node = from_bucket->nodes[i];
			int hash = hashtable->hash(node.key);
			if (!_inner_hashtable_add(buckets, capacity, hash, node.key, node.item))
			{
				arena_destroy(storage);
				return false;
			}
		}
	}

	arena_destroy(hashtable->storage);

	hashtable->storage = storage;
	hashtable->buckets = buckets;
	hashtable->bucket_count = capacity;
	return true;
}

boolean hashtable_contains(hashtable_t* hashtable, void* key)
{
	int hash = hashtable->hash(key);
	int bucketId = hash % hashtable->bucket_count;
	bucket_t* bucket = &hashtable->buckets[bucketId];

	for (int i = 0; i < bucket->count; i++)
		if (hashtable->match(bucket->nodes[i].key, key))
			return true;

	return false;
}

void* hashtable_get(hashtable_t* hashtable, void* key)
{
	int hash = hashtable->hash(key);
	int bucketId = hash % hashtable->bucket_count;
	bucket_t* bucket = &hashtable->buckets[bucketId];

	for (int i = 0; i < bucket->count; i++)
		if (hashtable->match(bucket->nodes[i].key, key))
			return bucket->nodes[i].item;

	return 0;
}

boolean hashtable_add(hashtable_t* hashtable, void* key, void* item)
{
	if (hashtable_contains(hashtable, key))
		return false;

	int hash = hashtable->hash(key);
	if (_inner_hashtable_add(hashtable->buckets, hashtable->bucket_count, hash, key, item))
	{
		hashtable->count++;
		return true;
	}

	return false;
}

boolean hashtable_remove(hashtable_t* hashtable, void* key)
{
	int hash = hashtable->hash(key);
	int bucketId = hash % hashtable->bucket_count;
	bucket_t* bucket = &hashtable->buckets[bucketId];

	for (int i = 0; i < bucket->count; i++)
	{
		if (hashtable->match(bucket->nodes[i].key, key))
		{
			size_t size = (bucket->count - i) * sizeof(node_t);
			memcpy(&bucket->nodes[i], &bucket->nodes[i + 1], size);
			bucket->count--;
			hashtable->count--;

			return true;
		}
	}

	return false;
}

void hashtable_clear(hashtable_t* hashtable)
{
	hashtable->count = 0;
	memset(hashtable->buckets, 0, sizeof(bucket_t) * hashtable->bucket_count);
}

void hashtable_destroy(hashtable_t* hashtable)
{
	arena_destroy(hashtable->storage);
}

hashtable_t* create_int_hashtable(int capacity)
{
	return create_hashtable(capacity, _key_hash_int, _key_match_int);
}
