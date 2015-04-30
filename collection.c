#include "arl.h"
#include "memory.h"

memoryArena_t* collections = NULL;

collection_t* createCollection(size_t initialSize, size_t itemSize)
{
	collection_t* collection = (collection_t*)alloc(&collections, sizeof(collection_t));
	collection->capacity = initialSize;
	collection->storage = create_arena(initialSize * sizeof(collectionNode_t));
	collection->itemStorage = create_arena(initialSize * itemSize);
	collection->head = 0;

	return collection;
}

void add(collection_t* collection, void* item)
{
	collectionNode_t* node = (collectionNode_t* )alloc(&collection->storage, sizeof(collectionNode_t));
	node->item = item;
	node->next = collection->head;
	collection->head = node;
}

void* newItem(collection_t* collection, size_t sizeofItem)
{
	void* item = alloc(&collection->itemStorage, sizeofItem);
	add(collection, item);
	return item;
}

void* getAt(collection_t* collection, uint index)
{
	collectionNode_t* p = collection->head;
	while (p && index)
	{
		p = p->next;
		index--;
	}

	return p->item;
}

int count(collection_t* collection)
{
	int count = 0;
	collectionNode_t* p = collection->head;
	while (p)
	{
		count++;
		p = p->next;
	}

	return count;
}