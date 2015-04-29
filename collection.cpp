#include "arl.h"
#include "platform.h"

memoryArena_t _collectionStorage = {};
memoryArena_t* collections = &_collectionStorage;

collection_t* 
createCollection(size_t initialSize, size_t itemSize)
{
	collection_t* collection = (collection_t*)arenaAlloc(&collections, sizeof(collection_t));
	collection->capacity = initialSize;
	collection->storage = allocateArena(initialSize * sizeof(collectionNode_t));
	collection->itemStorage = allocateArena(initialSize * itemSize);
	collection->head = 0;

	return collection;
}

void add(collection_t* collection, void* item)
{
	collectionNode_t* node = (collectionNode_t* )arenaAlloc(&collection->storage, sizeof(collectionNode_t));
	node->item = item;
	node->next = collection->head;
	collection->head = node;
}

void* newItem(collection_t* collection, size_t sizeofItem)
{
	void* item = arenaAlloc(&collection->itemStorage, sizeofItem);
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
	auto p = collection->head;
	while (p)
	{
		count++;
		p = p->next;
	}

	return count;
}