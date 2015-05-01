#include <string.h>
#include <stdlib.h>

#include "arl.h"
#include "platform.h"

internal
int monster(const char* key, char* value, gameState_t* game)
{
	if (_stricmp("monster", key) || !game->currentLevel.mobs)
		return 0;

	int targetId = atoi(value);
	int monsterId = 1;
	foreach(monster_t*, m, game->currentLevel.mobs)
	{
		if (monsterId == targetId)
		{
			char *next_token = NULL;
			char* p = strtok_s(value, " ", &next_token);
			m->position.x = atoi(next_token);

			p = strtok_s(NULL, " ", &next_token);
			m->position.y = atoi(next_token);
			
			p = strtok_s(NULL, " ", &next_token);
			m->energy = atoi(next_token);
			return 1;
		}

		monsterId++;
	}

	return 1;
}

internal
void door(const char* value, gameState_t* game)
{
	int targetDoorId = atoi(value);
	int doorId = 0;
	mapElement_t* p = game->currentLevel.map;
	while (p->type != END_OF_MAP)
	{
		if (isDoor(p))
		{
			doorId++;
			if (doorId == targetDoorId)
			{
				p->type = str_endswith(value, "OPEN")
					? OPEN_DOOR
					: DOOR;

				return;
			}
		}

		p++;
	}
}

internal
void setPlayerProperty(const char* key, char* value, player_t* player)
{
	if (str_equals("DAMAGE", key))
	{
		player->damage = atoi(value);
	}
	else if (str_equals("DEFENSE", key))
	{
		player->defense = atoi(value);
	}
	else if (str_equals("ATTACK", key))
	{
		player->attack = atoi(value);
	}
	else if (str_equals("HP", key))
	{
		player->hp = atoi(value);
	}
	else if (str_equals("POSITION", key))
	{
		char* context;
		char* token;

		token = strtok_s(value, " ", &context);
		player->position.x = atoi(token);

		token = strtok_s(NULL, " ", &context);
		player->position.y = atoi(token);
	}
}

internal
void loadPlayerItem(file_t* file, item_t* item)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_ITEM"))
		{
			return;
		}

		char* key;
		char* value;
		parseKeyValue(buffer, &key, &value);
		setItemProperty(key, value, item);
	}
}

internal
void loadPlayerInventory(file_t* file, player_t* player)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_INVENTORY"))
		{
			return;
		}
		
		item_t* item = (item_t*)newItem(player->inventory, sizeof(item_t));
		loadPlayerItem(file, item);
	}
}

internal
void loadPlayer(file_t* file, player_t* player)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_PLAYER"))
		{
			return;
		}

		if (str_startswith(buffer, "INVENTORY"))
		{
			loadPlayerInventory(file, player);
			continue;
		}

		char* key;
		char* value;
		parseKeyValue(buffer, &key, &value);
		setPlayerProperty(key, value, player);
	}
}

void beginParse(file_t* file, char* buffer, gameState_t* game)
{
	char* key;
	char* value;

	if (str_equals(buffer, "PLAYER"))
	{
		loadPlayer(file, &game->player);
	}
	else if (str_startswith(buffer, "MONSTER"))
	{
		parseKeyValue(buffer, &key, &value);
		int monsterId = atoi(value);

		monster_t* monster = (monster_t*)getAt(game->currentLevel.mobs, monsterId - 1);
		loadMonster(file, monster);
		return;
	}
	else if (str_equals(buffer, "DOOR"))
	{
		parseKeyValue(buffer, &key, &value);
		door(value, game);
	}
}

void loadGame(gameState_t* game)
{
	file_t* file;
	if (!openFileForRead("savegame.txt", &file))
		return;

	const char* line;
	char buffer_[512];
	char* buffer = buffer_;

	while ((line = readLine(file)) != NULL)
	{
		strncpy_s(buffer, 512, line, _TRUNCATE);
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		beginParse(file, buffer, game);
	}

	freeFile(file);
}