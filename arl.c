#include "arl.h"
#include "memory.h"
#include "platform.h"

#include <stdio.h>
#include <math.h>
#include <stdarg.h>

v2i _screenSize = { 20, 10 };
game_t _game = {0};
char* _statusMessage = 0;

internal
v2i _to_screen_coord(v2i pos)
{
	int xOffset = (80 - _screenSize.x) / 2;
	int yOffset = (25 - _screenSize.y) / 2;

	return (v2i){ pos.x + xOffset, pos.y + yOffset };
}

void statusf(char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	_statusMessage = (char*)transient_alloc(512);
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

char _get_floor_glyph(level_t* level, int x, int y)
{
	collection_t* floor = items_on_floor(level, x, y);
	stacked_item_t* stacked = collection_first(floor);
	return stacked ? stacked->item->glyph : '.';
}

internal void
_render_map(level_t* map)
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
				map_element_t* mapElement = get_map_element(map, x, y);
				switch (mapElement->type)
				{
				case FLOOR:
					c = _get_floor_glyph(map, x, y);
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

			draw_char(_to_screen_coord(p), c);
		}

	foreach(monster_t*, m, _game.current_level.mobs)
	{
		if (m->hp > 0)
		{
			v2i xy = { m->position.x - mapOffset.x, m->position.y - mapOffset.y };
			if (clamp(&xy, _screenSize.x, _screenSize.y))
				continue;

			draw_char(_to_screen_coord(xy), m->glyph);
		}
	}
}

internal
void _render_stats(player_t* player)
{
	int left = 60;
	draw_line((v2i)	{ left, 2 }, "PLAYER:");
	drawf_line((v2i){ left, 3 }, "HP: %d", player->hp);
	drawf_line((v2i){ left, 4 }, "Attack: %d", player->attack);
	drawf_line((v2i){ left, 5 }, "Defense: %d", player->defense);
	drawf_line((v2i){ left, 6 }, "Damage: %d", player->damage);
	drawf_line((v2i){ left, 6 }, "Currency: %ld/%02d/%02d", player->currency / 10000, (player->currency % 10000) / 100, player->currency % 100);

	if (player->target)
	{
		draw_line((v2i){ left, 7 }, "TARGET:");
		drawf_line((v2i){ left, 8 }, "HP: %d", player->target->hp);
		drawf_line((v2i){ left, 9 }, "Attack: %d", player->target->attack);
		drawf_line((v2i){ left, 10 }, "Defense: %d", player->target->defense);
		drawf_line((v2i){ left, 11 }, "Damage: %d", player->target->damage);
	}
}

internal
void _render_item(stacked_item_t* stacked, int x, int y)
{
	if (stacked->quantity > 1)
	{
		drawf_line((v2i){ x, y }, "(%d) %s", stacked->quantity, stacked->item->name);
	}
	else
	{
		drawf_line((v2i){ x, y }, "%s", stacked->item->name);
	}
}

internal
int _render_floor(level_t* current_level, player_t* player, int offset)
{
	int left = 60;

	collection_t* floor = items_on_floor(current_level, player->position.x, player->position.y);
	if (collection_any(floor))
	{
		draw_line((v2i){ left, offset++ }, "FLOOR:");
		foreach(stacked_item_t*, item, floor)
		{
			_render_item(item, left, offset++);
		}
	}

	return offset;
}

internal
int _render_inventory(player_t* player, int x, int y, const char* title)
{
	if (collection_any(player->inventory))
	{
		draw_line((v2i){ x, y++ }, title);
		foreach(stacked_item_t*, item, player->inventory)
		{
			_render_item(item, x + 3, y++);
		}
	}

	return y;
}

void
update_and_render()
{
	if (_game.player.hp <= 0)
	{
		draw_to_buffer("You are dead!");
		return;
	}

	draw_to_buffer(_statusMessage);
	_render_map(&_game.current_level);
	_render_stats(&_game.player);
	int offset = 13;
	offset = _render_inventory(&_game.player, 60, offset, "INVENTORY");
	offset = _render_floor(&_game.current_level, &_game.player, offset);

	if (_game.select_item.active)
	{
		makeDark();
		int y = 10;
		draw_line((v2i){ 30, y++ }, _game.select_item.title);
		foreach(stacked_item_t*, item, _game.select_item.items())
		{
			if (item == _game.select_item.selected)
				draw_char((v2i){ 27, y }, '>');

			_render_item(item, 30, y++);
		}
	}
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
			element_type_t e = level->map[i.y * level->size.x + i.x].type;
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
			element_type_t e = level->map[i.y * level->size.x + i.x].type;
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
boolean moveMob(game_t* game, monster_t* mob)
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
	
	if (clamp(&newPos, game->current_level.size.x, _game.current_level.size.y))
		goto badMove;

	if (intersects(game->player.position, newPos))
	{
		int attack = (int )(rand_r() * 1.5 * mob->attack);
		int defense = (int )(rand_r() * 1.5 * game->player.defense);
		debugf("monster rolled %d and player rolled %d", attack, defense);

		if (attack >= defense)
		{
			int damage = (int )(rand_r() * mob->damage);
			game->player.hp -= damage;
			statusf("Monster did %d dmg to player", damage);
		}

		return true;
	}

	foreach (monster_t*, m, game->current_level.mobs)
	{
		if (m == mob)
			continue;

		if (intersects(m->position, newPos))
			goto badMove;
	}

	element_type_t mapElement = get_map_element_type(&game->current_level, newPos.x, newPos.y);
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
void moveMobs(game_t* game)
{
	foreach (monster_t*, m, game->current_level.mobs)
	{
		if (m->hp <= 0)
			continue;

		if (canSee(&game->current_level, m->position, game->player.position))
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

		int attack = (int )(rand_r() * 1.5 * player->attack);
		int defense = ( int )(rand_r() * 1.5 * m->defense);
		debugf("player rolled %d and monster rolled %d", attack, defense);
		if (attack >= defense)
		{
			int damage = ( int )(rand_r() * player->damage);
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

internal
boolean _move_player(game_t* game, const game_input_t input)
{
	if (!(input.x_offset || input.y_offset))
		return false;

	if (game->select_item.active)
		return false;

	v2i newPos = { game->player.position.x + input.x_offset, game->player.position.y + input.y_offset };
	int mapElement = get_map_element_type(&game->current_level, newPos.x, newPos.y);
	switch (mapElement)
	{
		case 1:
			break;
		case 2:
			openDoor(&game->current_level, newPos);
			break;
		default:
			if (!bump(&game->player, newPos, game->current_level.mobs))
			{
				game->player.position = newPos;
			}
			break;
	}

	return true;
}

internal
collection_t* _player_inventory()
{
	return _game.player.inventory;
}

internal
collection_t* _items_on_floor_at_player_location()
{
	player_t* player = &_game.player;
	return items_on_floor(&_game.current_level, player->position.x, player->position.y);
}

internal
boolean _has_selection(select_item_t* select, collection_t* (*items)())
{
	return select->active && select->selected && select->items == items;
}

internal
void _begin_item_select(select_item_t* select, collection_t* (*items)(), boolean(*confirm)(game_t* game, stacked_item_t* item), const char* title)
{
	select->active = true;
	select->items = items;
	select->confirm = confirm;
	select->title = title;
	select->selected = collection_first(items());
}

internal _pickup_item(player_t* player, level_t* level, item_t* item)
{
	item = pickup_item(level, player->position.x, player->position.y, item);
	if (item)
	{
		stacked_add(player->inventory, item);
		statusf("Picked up %s", item->name);
	}
}

internal
boolean _drop_item(player_t* player, level_t* level, item_t* target)
{
	foreach(stacked_item_t*, stacked, player->inventory)
	{
		if (stacked->item == target)
		{
			item_t* item = stacked_remove(player->inventory, stacked);
			drop_item(level, item, player->position.x, player->position.y);
			statusf("Dropped %s", item->name);
			return true;
		}
	}
	
	return false;
}

internal
boolean _end_pickup_item(game_t* game, stacked_item_t* selected)
{
	return _pickup_item(&game->player, &game->current_level, selected->item);
}

internal
boolean _begin_pickup_item(game_t* game)
{
	player_t* player = &game->player;
	level_t* level = &game->current_level;
	collection_t* itemsOnFloor = items_on_floor(level, player->position.x, player->position.y);

	if (!itemsOnFloor)
	{
		statusf("There is nothing here");
		return false;
	}

	stacked_item_t* single = collection_single(itemsOnFloor);
	if (single)
	{
		_pickup_item(player, level, single->item);
		return true;
	}

	_begin_item_select(&game->select_item, _items_on_floor_at_player_location, _end_pickup_item, "SELECT ITEM TO PICK UP");
	return false;
}

internal
boolean _end_drop_item(game_t* game, stacked_item_t* selected)
{
	return _drop_item(&game->player, &game->current_level, selected->item);
}

internal
boolean _begin_drop_item(game_t* game)
{
	collection_t* inventory = game->player.inventory;
	if (!collection_any(inventory))
	{
		statusf("There is nothing to drop");
		return false;
	}

	stacked_item_t* single = collection_single(inventory);
	if (single)
	{
		return _drop_item(&game->player, &game->current_level, single->item);
	}

	_begin_item_select(&game->select_item, _player_inventory, _end_drop_item, "SELECT ITEM TO DROP");
	return false;
}

internal
boolean _process_item_dialog(game_t* game, game_input_t input)
{
	GAME_ACTION action = input.action;
	switch (action)
	{
	case GAME_ACTION_OK:
	{
		game->select_item.active = false;
		return game->select_item.confirm(game, game->select_item.selected);
	}
	case GAME_ACTION_CANCEL:
	{
		game->select_item.active = false;
		return false;
	}
	}

	stacked_item_t* next = 0;
	if (input.y_offset > 0)
	{
		next = collection_next(game->select_item.items(), game->select_item.selected);
	}
	else if (input.y_offset < 0)
	{
		next = collection_prev(game->select_item.items(), game->select_item.selected);
	}

	if (next)
	{
		game->select_item.selected = next;
	}

	return false;
}

internal
boolean _perform_action(game_t* game, game_input_t input)
{
	GAME_ACTION action = input.action;
	switch (action)
	{
		case GAME_ACTION_PICKUP:
		{
			return _begin_pickup_item(game);
		}
		case GAME_ACTION_DROP:
		{
			return _begin_drop_item(game);
		}
	}

	return false;
}

void process_input(const game_input_t input)
{
	if (input.action == GAME_ACTION_QUIT)
		return;

	boolean actionTaken;
	if (_game.select_item.active)
	{
		actionTaken = _process_item_dialog(&_game, input);
	}
	else
	{
		actionTaken = _move_player(&_game, input) | _perform_action(&_game, input);
	}

	if (actionTaken)
	{
		moveMobs(&_game);
	}

	save_game(&_game);
}

void init_game()
{
	rand_init(0);
	init_game_struct(&_game);
}