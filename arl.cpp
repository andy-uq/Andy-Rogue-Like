#include "arl.h"
#include "platform.h"

#include <stdio.h>
#include <math.h>
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
v2i moveRandom(v2i pos)
{
	v2i newPos = pos;

	sRandom *= 113;

	if (sRandom % 11 == 0) newPos.y--;
	else if (sRandom % 7 == 1) newPos.y++;
	else if (sRandom % 5 == 2) newPos.x--;
	else if (sRandom % 3 == 3) newPos.x++;

	return newPos;
}

#define abs(x) ((x) > 0 ? (x) : -(x))

internal 
bool canSee(level_t* level, v2i source, v2i target)
{
	v2i p = { abs(target.x - source.x), abs(target.y - source.y) };
	double m = p.x == 0 ? NAN : ( double )p.y / ( double )p.x;
	double a = 0;

	if (p.x > p.y)
	{
		int d = (target.x > source.x) ? 1 : -1;
		if (target.y < source.y) 
			m *= -1;

		for (v2i i = source; p.x > 0; p.x--)
		{
			auto e = level->map[i.y * level->size.x + i.x].type;
			if (e != OPEN_DOOR && e != FLOOR)
				return false;

			i.x += d;
			a += m;
			if (a > 1)
			{
				i.y += 1;
				a -= 1;
			}
			else if (a < -1)
			{
				i.y -= 1;
				a += 1;
			}
		}

		return true;
	}
	else
	{
		int d = (target.y > source.y) ? 1 : -1;
		if (target.x < source.x)
			m *= -1;

		for (v2i i = source; p.y > 0; p.y--)
		{
			auto e = level->map[i.y * level->size.x + i.x].type;
			if (e != OPEN_DOOR && e != FLOOR)
				return false;

			i.y += d;
			a += m;
			if (a > 1)
			{
				i.x += 1;
				a -= 1;
			}
			else if (a < -1)
			{
				i.x -= 1;
				a += 1;
			}
		}

		return true;
	}
}

internal 
v2i moveTowards(v2i from, v2i towards)
{
	v2i newPos = from;
	v2i p = { towards.x - newPos.x, towards.y - newPos.y };

	if (p.x > 0) newPos.x++;
	if (p.x < 0) newPos.x--;
	if (p.y > 0) newPos.y++;
	if (p.y < 0) newPos.y--;
	
	return newPos;
}

internal
bool intersects(v2i source, v2i target)
{
	return (source.x == target.x && source.y == target.y);
}

internal
bool moveMob(gameState_t* game, monster_t* mob)
{
	v2i newPos;
	
	if (mob->target.x)
	{
		newPos = moveTowards(mob->position, mob->target);
	}
	else
	{
		return false;
	}
	
	if (clamp(&newPos, game->currentLevel.size.x, _game.currentLevel.size.y))
		goto badMove;

	if (intersects(game->charPos, newPos))
		goto badMove;

	for (monster_t* m = game->currentLevel.mobs; m->glyph; m++)
	{
		if (m == mob)
			continue;

		if (intersects(m->position, newPos))
			goto badMove;
	}

	elementType_t mapElement = getMapElement(&game->currentLevel, newPos);
	if (mapElement == OPEN_DOOR || mapElement == FLOOR)
	{
		mob->position = newPos;
		if (intersects(mob->position, mob->target))
		{
			mob->target = { 0, 0 };
		}

		return true;
	}

badMove:
	return false;
}

internal
void moveMobs(gameState_t* game)
{
	for (monster_t* m = game->currentLevel.mobs; m->glyph; m++)
	{
		if (canSee(&game->currentLevel, m->position, game->charPos))
		{
			m->target = game->charPos;
		}
		
		m->energy = (m->energy + m->speed > 100) ? 100 : m->energy + m->speed;
		if (m->energy > 10)
		{
			if (moveMob(game, m))
			{
				m->energy -= 10;
			}
		}
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

	moveMobs(&_game);
	saveGame(&_game);
}

void initGame()
{
	initGame(&_game);
}