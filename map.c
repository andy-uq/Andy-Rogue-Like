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
	assert(get_map_element(level, x, y));
	map_item_t* mapItem = collection_new_item(level->items, sizeof(*mapItem));
	mapItem->x = x;
	mapItem->y = y;
	mapItem->item = item;
}

item_t* pickup_item(level_t* level, int x, int y)
{
	foreach(map_item_t*, i, level->items)
	{
		if (i->x == x && i->y == y)
		{
			collection_remove(level->items, i);
			return i->item;
		}
	}

	return 0;
}

collection_t* items_on_floor(level_t* level, int x, int y)
{
	collection_t* collection = transient_collection(10, sizeof(item_t));
	foreach(map_item_t*, i, level->items)
	{
		if (i->x == x && i->y == y)
			collection_push(collection, i->item);
	}

	return collection;
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
