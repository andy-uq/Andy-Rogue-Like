#include <stdio.h>
#include <stdarg.h>

#include "arl.h"
#include "platform.h"

internal
void writeFileComment(file_t* file, const char* commentFormat, ...)
{
	char comment[508];
	char line[512];
	va_list args;
	va_start(args, commentFormat);
	vsnprintf_s(comment, sizeof(comment), _TRUNCATE, commentFormat, args);
	va_end(args);

	char* write = line;
	*(write++) = '#';
	*(write++) = ' ';

	write = str_append(write, comment);

	*(write++) = '\r';
	*(write++) = '\n';
	*(write++) = 0;

	file_write(file, line);
}

internal
void writeFileLine(file_t* file, const char* format, ...)
{
	char line[512];
	va_list args;
	va_start(args, format);
	vsnprintf_s(line, sizeof(line), _TRUNCATE, format, args);
	va_end(args);

	char* write = line;
	while (*write)
		write++;

	*(write++) = '\r';
	*(write++) = '\n';
	*(write++) = 0;

	file_write(file, line);
}

internal
void writeFileKeyValue(file_t* file, const char* key, const char* valueFormat, ...)
{
	char value[256];
	char line[512];
	va_list args;
	va_start(args, valueFormat);
	vsnprintf_s(value, sizeof(value), _TRUNCATE, valueFormat, args);
	va_end(args);

	char* write = str_append(line, key);
	*(write++) = '=';
	write = str_append(write, value);
	*(write++) = '\r';
	*(write++) = '\n';
	*(write++) = 0;

	file_write(file, line);
}

internal 
void saveDoors(file_t* saveGame, map_element_t* map)
{
	writeFileComment(saveGame, "DOORS");

	int doorId = 1;
	for (map_element_t* p = map; p->type != END_OF_MAP; p++)
	{
		if (is_door(p))
		{
			writeFileKeyValue(saveGame, "DOOR", "%d %s", doorId, (p->type == DOOR) ? "CLOSED" : "OPEN");
			doorId++;
		}
	}

	writeFileComment(saveGame, "END_DOORS");
}

internal 
void saveMonsters(file_t* saveGame, collection_t* mobs)
{
	if (!mobs)
		return;

	writeFileComment(saveGame, "MONSTERS");

	int monsterId = 1;
	foreach(monster_t*, m, mobs)
	{
		writeFileKeyValue(saveGame, "MONSTER", "%d", monsterId);
		writeFileKeyValue(saveGame, "POSITION", "%d %d", m->position.x, m->position.y);
		writeFileKeyValue(saveGame, "HP", "%d", m->hp);
		writeFileKeyValue(saveGame, "ATTACK", "%d", m->attack);
		writeFileKeyValue(saveGame, "DEFENSE", "%d", m->defense);
		writeFileKeyValue(saveGame, "DAMAGE", "%d", m->damage);
		writeFileLine(saveGame, "END_MONSTER");
		monsterId++;
	}

	writeFileComment(saveGame, "MONSTERS");
}

void savePlayer(file_t* saveGame, player_t* player)
{
	writeFileLine(saveGame, "PLAYER");
	writeFileKeyValue(saveGame, "POSITION", "%d %d", player->position.x, player->position.y);
	writeFileKeyValue(saveGame, "HP", "%d", player->hp);
	writeFileKeyValue(saveGame, "ATTACK", "%d", player->attack);
	writeFileKeyValue(saveGame, "DEFENSE", "%d", player->defense);
	writeFileKeyValue(saveGame, "DAMAGE", "%d", player->damage);

	writeFileLine(saveGame, "INVENTORY");
	foreach(item_t*, item, player->inventory)
	{
		writeFileKeyValue(saveGame, "ITEM", "%d", item->id);
		writeFileLine(saveGame, "END_ITEM");
	}
	writeFileLine(saveGame, "END_INVENTORY");

	writeFileLine(saveGame, "END_PLAYER");
}

void save_game(game_t* game)
{
	file_t* saveGame;
	if (!file_open_for_write("savegame.txt", &saveGame))
		return;

	writeFileComment(saveGame, "Savegame v1.0");
	savePlayer(saveGame, &game->player);
	saveDoors(saveGame, game->current_level.map);
	saveMonsters(saveGame, game->current_level.mobs);

	writeFileComment(saveGame, "EOF");
	file_free(saveGame);
}
