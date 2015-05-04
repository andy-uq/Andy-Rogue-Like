#include "arl.h"

boolean is_door(map_element_t* e)
{
	return
		e->type == DOOR
		|| e->type == OPEN_DOOR;
}

map_element_t* get_map_element(level_t* level, int x, int y)
{
	v2i pos = { x, y };
	if (clamp(&pos, level->size.x, level->size.y))
		return 0;

	return &level->map[(y * level->size.x) + x];
}

element_type_t get_map_element_type(level_t* level, int x, int y)
{
	map_element_t* tile = get_map_element(level, x, y);
	return tile ? tile->type : -1;
}
