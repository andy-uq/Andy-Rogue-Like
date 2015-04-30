#include "arl.h"

boolean isDoor(mapElement_t* e)
{
	return
		e->type == DOOR
		|| e->type == OPEN_DOOR;
}

elementType_t getMapElement(level_t* level, int x, int y)
{
	v2i pos = { x, y };
	if (clamp(&pos, level->size.x, level->size.y))
		return (elementType_t)-1;

	return level->map[(y * level->size.x) + x].type;
}
