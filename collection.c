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

void collection_add(collection_t* collection, void* item)
{
	collection_node_t* node = (collection_node_t* )arena_alloc(&collection->node_storage, sizeof(collection_node_t));
	node->item = item;
	node->next = collection->head;
	collection->head = node;
	collection->count++;
}

void* collection_new_item(collection_t* collection, size_t sizeofItem)
{
	void* item = arena_alloc(&collection->item_storage, sizeofItem);
	collection_add(collection, item);
	return item;
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