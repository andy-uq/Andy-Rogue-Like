#include <string.h>

#include "arl.h"
#include "platform.h"

internal
int readMonster(const char* filename, level_t* level)
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return 0;

	int monsterCount = 0;

	char* line;
	while ((line = readLine(file)) != NULL)
	{
		if (str_startswith(line, "END_MONSTER"))
			monsterCount++;
	}

	seek(file, 0L);
	level->mobs = (monster_t* )allocate(sizeof(monster_t) * (monsterCount + 1));

	loadMonsters(file, level->mobs);
	freeFile(file);

	return monsterCount;
}

internal
int readMap(const char* filename, gameState_t* game, level_t* level)
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return 0;

	void* memory = allocate((file->size + 1) * sizeof(mapElement_t));
	level->map = (mapElement_t*)memory;

	mapElement_t* ptr = level->map;

	char* line;
	while ((line = readLine(file)) != NULL)
	{
		int xOffset = 0;
		while (*line)
		{
			*ptr = {};
			switch (*line)
			{
			case '#':
				ptr->type = WALL;
				break;
			case '/':
				ptr->type = DOOR;
				break;
			case '=':
				ptr->type = OPEN_DOOR;
				break;
			case '@':
				game->charPos.x = xOffset;
				game->charPos.y = level->size.y;
				break;
			}

			line++;
			xOffset++;
			ptr++;
		}

		level->size.y++;
		if (xOffset > level->size.x)
			level->size.x = xOffset;
	}

	ptr->type = END_OF_MAP;

	freeFile(file);
	return 1;
}

internal
level_t* readLevel(gameState_t* game, level_t* level)
{
	file_t* file;
	if (!openFileForRead(level->filename, &file))
		return 0;

	const char* line;
	char buffer_[512];
	char* buffer = buffer_;

	while ((line = readLine(file)) != NULL)
	{
		strncpy_s(buffer, 512, line, _TRUNCATE);
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		char* value;
		if (parseKey(buffer, "MAP", &value))
		{
			if (!readMap(value, game, level))
				return NULL;

			continue;
		}

		if (parseKey(buffer, "MONSTER", &value))
		{
			if (!readMonster(value, level))
				return NULL;
		}
	}

	freeFile(file);
	return level;
}

void initGame(gameState_t* game)
{
	level_t* level = &game->currentLevel;
	game->charPos = { 1, 1 };
	
	level->filename = "level01.txt";

	readLevel(game, level);
	loadGame(game);
}