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
	level->mobs = createCollection(monsterCount, sizeof(monster_t));

	loadMonsters(file, level->mobs);
	freeFile(file);

	return monsterCount;
}

internal
item_t* readItem(const char* filename)
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return 0;

	int itemCount = 0;

	char* line;
	while ((line = readLine(file)) != NULL)
	{
		if (str_startswith(line, "END_ITEM"))
			itemCount++;
	}

	seek(file, 0L);
	auto items = (item_t*)allocate(sizeof(item_t) * (itemCount + 1));

	loadItems(file, items);
	freeFile(file);

	return items;
}

internal
int readLevelItem(const char* filename, level_t* level)
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return 0;

	char* line;
	while ((line = readLine(file)) != NULL)
	{
		str_trim(&line);

		if (line[0] == '#')
			continue;

		// TODO: read item and place at mapElement
		level->items = 0;
	}


	freeFile(file);

	return 1;
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
				game->player.position.x = xOffset;
				game->player.position.y = level->size.y;
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

		if (parseKey(buffer, "ITEM", &value))
		{
			if (!readLevelItem(value, level))
				return NULL;
		}
	}

	freeFile(file);
	return level;
}

internal
void initItems(gameState_t* game)
{
	game->items = readItem("items.txt");
}

internal
void initPlayer(player_t* player)
{
	player->position = { 1, 1 };
	player->attack = 100;
	player->defense = 100;
	player->hp = 20;
	player->damage = 10;
}

void initGame(gameState_t* game)
{
	initPlayer(&game->player);
	initItems(game);

	level_t* level = &game->currentLevel;
	level->filename = "level01.txt";
	readLevel(game, level);

	loadGame(game);
}