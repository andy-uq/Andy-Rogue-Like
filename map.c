#include "arl.h"

boolean is_door(map_element_t* e)
{
	return
		e->type == DOOR
		|| e->type == OPEN_DOOR;
}

element_type_t get_map_element(level_t* level, int x, int y)
{
	v2i pos = { x, y };
	if (clamp(&pos, level->size.x, level->size.y))
		return (element_type_t)-1;

	return level->map[(y * level->size.x) + x].type;
}
