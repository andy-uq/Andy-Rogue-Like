#include "arl.h"
#include "memory.h"

arena_t* collections = NULL;

collection_t* create_collection(size_t initialSize, size_t itemSize)
{
	collection_t* collection = (collection_t*)arena_alloc(&collections, sizeof(collection_t));
	collection->capacity = initialSize;
	collection->node_storage = arena_create(initialSize * sizeof(collection_node_t));
	collection->item_storage = arena_create(initialSize * itemSize);
	collection->head = 0;
	collection->count = 0;

	return collection;
}

internal
collection_node_t* _alloc_node(collection_t* collection)
{
	if (collection->free_list == 0)
		return arena_alloc(&collection->node_storage, sizeof(collection_node_t));

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

boolean collection_any(collection_t* collection)
{
	collection_node_t* p = collection ? collection->head : 0;
	return p != 0;
}

void* collection_get_at(collection_t* collection, uint index)
{
	collection_node_t* p = collection->head;
	while (p && index)
	{
		p = p->next;
		index--;
	}

	return p->item;
}

int collection_count(collection_t* collection)
{
	return collection->count;
}