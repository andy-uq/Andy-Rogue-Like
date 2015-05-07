#include <assert.h>

#include "arl.h"
#include "memory.h"

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