#include "arl.h"
#include "platform.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

v2i _screenSize = { 20, 10 };
gameState_t _game = {};

internal
v2i toScreenCoord(v2i pos)
{
	int xOffset = (80 - _screenSize.x) / 2;
	int yOffset = (25 - _screenSize.y) / 2;

	return { pos.x + xOffset, pos.y + yOffset };
}

bool clamp(v2i* pos, int x, int y)
{
	bool clamped = false;

	if (pos->x < 0)
	{
		pos->x = 0;
		clamped = true;
	}

	if (pos->y < 0)
	{
		pos->y = 0;
		clamped = true;
	}

	if (pos->y > y)
	{
		pos->y = y;
		clamped = true;
	}

	if (pos->x > x)
	{
		pos->x = x;
		clamped = true;
	}

	return clamped;
}

internal void
renderMap(level_t* map)
{
	v2i mapOffset = { _game.charPos.x - (_screenSize.x / 2), _game.charPos.y - (_screenSize.y / 2) };
	clamp(&mapOffset, map->size.x - _screenSize.x, map->size.y - _screenSize.y);

	v2i p = {};
	for (p.x = 0; p.x < _screenSize.x; p.x++)
		for (p.y = 0; p.y < _screenSize.y; p.y++)
		{
			int x = p.x + mapOffset.x;
			int y = p.y + mapOffset.y;

			char c;
			if (_game.charPos.x == x && _game.charPos.y == y)
			{
				c = '@';
			}
			else
			{
				int mapElement = getMapElement(map, x, y);
				switch (mapElement)
				{
				case FLOOR:
					c = '.';
					break;
				case DOOR:
					c = '/';
					break;
				case OPEN_DOOR:
					c = '=';
					break;
				case WALL:
				default:
					c = '#';
					break;
				}
			}

			drawCharAt(toScreenCoord(p), c);
		}

	for (monster_t* m = _game.currentLevel.mobs; m->glyph; m++)
	{
		v2i xy = { m->position.x - mapOffset.x, m->position.y - mapOffset.y };
		if (clamp(&xy, _screenSize.x, _screenSize.y))
			continue;

		drawCharAt(toScreenCoord(xy), m->glyph);
	}
}

void
updateAndRender()
{
	renderMap(&_game.currentLevel);
}

int sRandom = 1337;

internal
void moveMob(v2i* m)
{
	v2i newPos = *m;

	sRandom *= 113;

	if (sRandom % 11 == 0) newPos.y--;
	else if (sRandom % 7 == 1) newPos.y++;
	else if (sRandom % 5 == 2) newPos.x--;
	else if (sRandom % 3 == 3) newPos.x++;

	if (clamp(&newPos, _game.currentLevel.size.x, _game.currentLevel.size.y))
		return;

	elementType_t mapElement = getMapElement(&_game.currentLevel, newPos);
	if (mapElement == OPEN_DOOR || mapElement == FLOOR)
	{
		*m = newPos;
	}
}

internal
void moveMobs()
{
	for (monster_t* m = _game.currentLevel.mobs; m->glyph; m++)
	{
		moveMob(&m->position);
	}
}

internal
int openDoor(level_t* level, v2i pos)
{
	if (clamp(&pos, level->size.x, level->size.y))
		return 0;

	level->map[(pos.y * level->size.x) + pos.x].type = OPEN_DOOR;
	return 1;
}

void
processInput(const game_input input)
{
	v2i new_pos = { _game.charPos.x + input.xOffset, _game.charPos.y + input.yOffset };
	int mapElement = getMapElement(&_game.currentLevel, new_pos);
	switch (mapElement)
	{
	case 1:
		return;
	case 2:
		openDoor(&_game.currentLevel, new_pos);
		return;
	default:
		_game.charPos = new_pos;
		break;
	}

	moveMobs();
	saveGame(&_game);
}

void initGame()
{
	initGame(&_game);
}