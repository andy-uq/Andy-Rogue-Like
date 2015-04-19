#include "arl.h"

bool isDoor(mapElement_t* e)
{
	return
		e->type == DOOR
		|| e->type == OPEN_DOOR;
}

elementType_t getMapElement(level_t* level, int x, int y)
{
	return level->map[(y * level->size.x) + x].type;
}

elementType_t getMapElement(level_t* level, v2i pos)
{
	if (clamp(&pos, level->size.x, level->size.y))
		return (elementType_t)-1;

	return getMapElement(level, pos.x, pos.y);
}
