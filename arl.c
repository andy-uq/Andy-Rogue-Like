#include "arl.h"
#include "memory.h"
#include "platform.h"

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>

v2i _screenSize = { 20, 10 };
gameState_t _game = {0};
char* _statusMessage = 0;

internal
v2i toScreenCoord(v2i pos)
{
	int xOffset = (80 - _screenSize.x) / 2;
	int yOffset = (25 - _screenSize.y) / 2;

	return (v2i){ pos.x + xOffset, pos.y + yOffset };
}

void statusf(char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	_statusMessage = (char*)trans_alloc(512);
	vsprintf_s(_statusMessage, 512, format, argp);
	debug(_statusMessage);
	va_end(argp);
}

boolean clamp(v2i* pos, int x, int y)
{
	boolean clamped = false;

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
	v2i mapOffset = { _game.player.position.x - (_screenSize.x / 2), _game.player.position.y - (_screenSize.y / 2) };
	clamp(&mapOffset, map->size.x - _screenSize.x, map->size.y - _screenSize.y);

	v2i p = {0};
	for (p.x = 0; p.x < _screenSize.x; p.x++)
		for (p.y = 0; p.y < _screenSize.y; p.y++)
		{
			int x = p.x + mapOffset.x;
			int y = p.y + mapOffset.y;

			char c;
			if (_game.player.position.x == x && _game.player.position.y == y)
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

	foreach(monster_t*, m, _game.currentLevel.mobs)
	{
		if (m->hp > 0)
		{
			v2i xy = { m->position.x - mapOffset.x, m->position.y - mapOffset.y };
			if (clamp(&xy, _screenSize.x, _screenSize.y))
				continue;

			drawCharAt(toScreenCoord(xy), m->glyph);
		}
	}
}

internal
void renderStats(player_t* player)
{
	drawLineAt((v2i){ 65, 2 }, "PLAYER:");
	drawfLineAt((v2i){ 65, 3 }, "HP: %d", player->hp);
	drawfLineAt((v2i){ 65, 4 }, "Attack: %d", player->attack);
	drawfLineAt((v2i){ 65, 5 }, "Defense: %d", player->defense);
	drawfLineAt((v2i){ 65, 6 }, "Damage: %d", player->damage);

	if (player->target)
	{
		drawLineAt((v2i){ 65, 7 }, "TARGET:");
		drawfLineAt((v2i){ 65, 8 }, "HP: %d", player->target->hp);
		drawfLineAt((v2i){ 65, 9 }, "Attack: %d", player->target->attack);
		drawfLineAt((v2i){ 65, 10 }, "Defense: %d", player->target->defense);
		drawfLineAt((v2i){ 65, 11 }, "Damage: %d", player->target->damage);
	}
}

void
updateAndRender()
{
	if (_game.player.hp <= 0)
	{
		drawToBuffer("You are dead!");
		return;
	}

	drawToBuffer(_statusMessage);
	renderMap(&_game.currentLevel);
	renderStats(&_game.player);
}

#define abs(x) ((x) > 0 ? (x) : -(x))

internal 
boolean canSee(level_t* level, v2i source, v2i target)
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
			elementType_t e = level->map[i.y * level->size.x + i.x].type;
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
			elementType_t e = level->map[i.y * level->size.x + i.x].type;
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
boolean intersects(v2i source, v2i target)
{
	return (source.x == target.x && source.y == target.y);
}

internal
boolean moveMob(gameState_t* game, monster_t* mob)
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

	if (intersects(game->player.position, newPos))
	{
		int attack = (int )(genrand_real1() * 1.5 * mob->attack);
		int defense = (int )(genrand_real1() * 1.5 * game->player.defense);
		debugf("monster rolled %d and player rolled %d", attack, defense);

		if (attack >= defense)
		{
			int damage = (int )(genrand_real1() * mob->damage);
			game->player.hp -= damage;
			statusf("Monster did %d dmg to player", damage);
		}

		return true;
	}

	foreach (monster_t*, m, game->currentLevel.mobs)
	{
		if (m == mob)
			continue;

		if (intersects(m->position, newPos))
			goto badMove;
	}

	elementType_t mapElement = getMapElement(&game->currentLevel, newPos.x, newPos.y);
	if (mapElement == OPEN_DOOR || mapElement == FLOOR)
	{
		mob->position = newPos;
		if (intersects(mob->position, mob->target))
		{
			mob->target = (v2i ){ 0, 0 };
		}

		return true;
	}

badMove:
	return false;
}

internal
void moveMobs(gameState_t* game)
{
	foreach (monster_t*, m, game->currentLevel.mobs)
	{
		if (m->hp <= 0)
			continue;

		if (canSee(&game->currentLevel, m->position, game->player.position))
		{
			m->target = game->player.position;
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

internal
boolean bump(player_t* player, v2i pos, collection_t* mobs)
{
	int monsterId = 0;
	foreach (monster_t*, m, mobs)
	{
		monsterId++;
		if (m->hp <= 0 || !intersects(pos, m->position))
			continue;

		player->target = m;

		int attack = (int )(genrand_real1() * 1.5 * player->attack);
		int defense = ( int )(genrand_real1() * 1.5 * m->defense);
		debugf("player rolled %d and monster rolled %d", attack, defense);
		if (attack >= defense)
		{
			int damage = ( int )(genrand_real1() * player->damage);
			m->hp -= damage;
			if (m->hp > 0)
			{
				statusf("Did %d dmg to monster %d", damage, monsterId);
			}
			else
			{
				statusf("Killed monster %d", monsterId);
			}
		}

		return true;
	}

	return false;
}

void processInput(const game_input input)
{
	if (!(input.xOffset || input.yOffset))
		return;
	
	v2i newPos = { _game.player.position.x + input.xOffset, _game.player.position.y + input.yOffset };
	int mapElement = getMapElement(&_game.currentLevel, newPos.x, newPos.y);
	switch (mapElement)
	{
	case 1:
		return;
	case 2:
		openDoor(&_game.currentLevel, newPos);
		return;
	default:
		if (!bump(&_game.player, newPos, _game.currentLevel.mobs))
		{
			_game.player.position = newPos;
		}
		break;
	}

	moveMobs(&_game);
	saveGame(&_game);
}

void initGame()
{
	init_genrand(0);
	initGameStruct(&_game);
}