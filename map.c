#include <assert.h>
#include "arl.h"

boolean is_door(map_element_t* e)
{
	return
		e->type == DOOR
		|| e->type == OPEN_DOOR;
}

void drop_item(level_t* level, item_t* item, int x, int y)
{
	collection_t* itemsOnFloor = items_on_floor(level, x, y);
	if (!itemsOnFloor)
	{
		itemsOnFloor = collection_from_arena(&level->storage);
		
		v2i pos = { x, y };
		hashtable_add(level->items, &pos, itemsOnFloor);
	}

	collection_push(itemsOnFloor, item);
}

item_t* pickup_item(level_t* level, int x, int y)
{
	collection_t* itemsOnFloor = items_on_floor(level, x, y);
	if (itemsOnFloor)
	{
		return collection_pop(itemsOnFloor);
	}

	return 0;
}

collection_t* items_on_floor(level_t* level, int x, int y)
{
	assert(get_map_element(level, x, y));
	v2i pos = { x, y };
	collection_t* itemsOnFloor = hashtable_get(level->items, &pos);
	return itemsOnFloor;
}

map_element_t* get_map_element(level_t* level, int x, int y)
{
	v2i pos = { x, y };
	if (clamp(&pos, level->size.x, level->size.y))
		return 0;

	return &level->map[(y * level->size.x) + x];
}

map_element_t* get_player_tile(level_t* level, player_t* player)
{
	return get_map_element(level, player->position.x, player->position.y);
}

element_type_t get_map_element_type(level_t* level, int x, int y)
{
	map_element_t* tile = get_map_element(level, x, y);
	return tile ? tile->type : -1;
}
