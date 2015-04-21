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
	vsnprintf_s(comment, sizeof(comment), commentFormat, args);
	va_end(args);

	char* write = line;
	*(write++) = '#';
	*(write++) = ' ';

	write = str_append(write, comment);

	*(write++) = '\r';
	*(write++) = '\n';
	*(write++) = 0;

	writeLine(file, line);
}

internal
void writeFileKeyValue(file_t* file, const char* key, const char* valueFormat, ...)
{
	char value[256];
	char line[512];
	va_list args;
	va_start(args, valueFormat);
	vsnprintf_s(value, sizeof(value), valueFormat, args);
	va_end(args);

	char* write = str_append(line, key);
	*(write++) = '=';
	write = str_append(write, value);
	*(write++) = '\r';
	*(write++) = '\n';
	*(write++) = 0;

	writeLine(file, line);
}

internal void saveDoors(file_t* saveGame, mapElement_t* map)
{
	writeFileComment(saveGame, "DOORS");

	int doorId = 1;
	for (mapElement_t* p = map; p->type != END_OF_MAP; p++)
	{
		if (isDoor(p))
		{
			writeFileKeyValue(saveGame, "DOOR", "%d %s", doorId, (p->type == DOOR) ? "CLOSED" : "OPEN");
			doorId++;
		}
	}

	writeFileComment(saveGame, "END_DOORS");
}

internal void saveMonsters(file_t* saveGame, monster_t* mobs)
{
	if (!mobs)
		return;

	writeFileComment(saveGame, "MONSTERS");

	int monsterId = 1;
	for (monster_t* m = mobs; m->glyph; m++)
	{
		writeFileKeyValue(saveGame, "MONSTER", "%d %d %d %d", monsterId, m->position.x, m->position.y, m->energy);
		monsterId++;
	}

	writeFileComment(saveGame, "MONSTERS");
}

void saveGame(gameState_t* game)
{
	file_t* saveGame;
	if (!openFileForWrite("savegame.txt", &saveGame))
		return;

	writeFileComment(saveGame, "Savegame v1.0");
	writeFileKeyValue(saveGame, "POS_X", "%d", game->charPos.x);
	writeFileKeyValue(saveGame, "POS_Y", "%d", game->charPos.y);

	saveDoors(saveGame, game->currentLevel.map);
	saveMonsters(saveGame, game->currentLevel.mobs);

	writeFileComment(saveGame, "EOF");
	freeFile(saveGame);
}
