#include <string.h>
#include <stdlib.h>

#include "arl.h"
#include "platform.h"

internal
void door(const char* value, game_t* game)
{
	int targetDoorId = atoi(value);
	int doorId = 0;
	map_element_t* p = game->current_level.map;
	while (p->type != END_OF_MAP)
	{
		if (is_door(p))
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
	while ((buffer = file_read(file)) != NULL)
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
		parse_key_value(buffer, &key, &value);
		set_item_property(key, value, item);
	}
}

internal
void loadPlayerInventory(file_t* file, player_t* player)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_INVENTORY"))
		{
			return;
		}
		
		item_t* item = (item_t*)collection_new_item(player->inventory, sizeof(item_t));
		loadPlayerItem(file, item);
	}
}

internal
void loadPlayer(file_t* file, player_t* player)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
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
		parse_key_value(buffer, &key, &value);
		setPlayerProperty(key, value, player);
	}
}

void beginParse(file_t* file, char* buffer, game_t* game)
{
	char* key;
	char* value;

	if (str_equals(buffer, "PLAYER"))
	{
		loadPlayer(file, &game->player);
	}
	else if (str_startswith(buffer, "MONSTER"))
	{
		parse_key_value(buffer, &key, &value);
		int monsterId = atoi(value);

		monster_t* monster = (monster_t*)collection_get_at(game->current_level.mobs, monsterId - 1);
		load_monster(file, monster);
		return;
	}
	else if (str_equals(buffer, "DOOR"))
	{
		parse_key_value(buffer, &key, &value);
		door(value, game);
	}
}

void load_game(game_t* game)
{
	file_t* file;
	if (!file_open_for_read("savegame.txt", &file))
		return;

	const char* line;
	char buffer_[512];
	char* buffer = buffer_;

	while ((line = file_read(file)) != NULL)
	{
		strncpy_s(buffer, 512, line, _TRUNCATE);
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		beginParse(file, buffer, game);
	}

	file_free(file);
}