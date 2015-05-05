#include <string.h>
#include <stdlib.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

internal
int read_monster(const char* filename, level_t* level)
{
	file_t* file;
	if (!file_open_for_read(filename, &file))
		return 0;

	int monsterCount = 0;

	char* line;
	while ((line = file_read(file)) != NULL)
	{
		if (str_startswith(line, "END_MONSTER"))
			monsterCount++;
	}

	file_seek(file, 0L);
	level->mobs = create_collection(monsterCount, sizeof(monster_t));

	load_monsters(file, level->mobs);
	file_free(file);

	return monsterCount;
}

internal
collection_t* read_item(const char* filename)
{
	file_t* file;
	if (!file_open_for_read(filename, &file))
		return 0;

	int itemCount = 0;

	char* line;
	while ((line = file_read(file)) != NULL)
	{
		if (str_startswith(line, "END_ITEM"))
			itemCount++;
	}

	file_seek(file, 0L);
	collection_t* items = create_collection(itemCount, sizeof(item_t));

	load_items(file, items);
	file_free(file);

	return items;
}

internal
int read_level_item(const char* filename, collection_t* items, level_t* level)
{
	file_t* file;
	if (!file_open_for_read(filename, &file))
		return 0;

	char *buffer, *value;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (!parse_value_if_match(buffer, "ITEM", &value))
			continue;

		char *context, *token;
		int id, x, y;

		token = strtok_s(value, " ", &context);
		id = atoi(token);

		token = strtok_s(NULL, " ", &context);
		x = atoi(token);

		token = strtok_s(NULL, " ", &context);
		y = atoi(token);

		item_t* item = find_item(items, id);
		if (!item)
			continue;

		map_element_t* map = get_map_element(level, x, y);
		if (!map->items)
			map->items = create_collection(0, 0);

		collection_add(map->items, item);
	}

	file_free(file);
	return 1;
}

internal
int read_map(const char* filename, game_t* game, level_t* level)
{
	file_t* file;
	if (!file_open_for_read(filename, &file))
		return 0;

	void* memory = arena_alloc(&level->storage, (file->size + 1) * sizeof(map_element_t));
	level->map = (map_element_t*)memory;

	map_element_t* ptr = level->map;

	char* line;
	while ((line = file_read(file)) != NULL)
	{
		int xOffset = 0;
		while (*line)
		{
			*ptr = (map_element_t ){0};
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

	file_free(file);
	return 1;
}

internal
level_t* read_level(game_t* game, level_t* level)
{
	file_t* file;
	if (!file_open_for_read(level->filename, &file))
		return 0;

	const char* line;
	char buffer_[512];
	char* buffer = buffer_;

	while ((line = file_read(file)) != NULL)
	{
		strncpy_s(buffer, 512, line, _TRUNCATE);
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		char* value;
		if (parse_value_if_match(buffer, "MAP", &value))
		{
			if (!read_map(value, game, level))
				return NULL;

			continue;
		}

		if (parse_value_if_match(buffer, "MONSTER", &value))
		{
			if (!read_monster(value, level))
				return NULL;
		}

		if (parse_value_if_match(buffer, "ITEM", &value))
		{
			if (!read_level_item(value, game->items, level))
				return NULL;
		}
	}

	file_free(file);
	return level;
}

internal
void items_init(game_t* game)
{
	game->items = read_item("items.txt");
}

internal
void player_init(player_t* player)
{
	player->position = (v2i ) { 1, 1 };
	player->attack = 100;
	player->defense = 100;
	player->hp = 20;
	player->damage = 10;
	player->inventory = create_collection(10, sizeof(item_t));
}

void init_game_struct(game_t* game)
{
	player_init(&game->player);
	items_init(game);

	level_t* level = &game->current_level;
	level->filename = "level01.txt";
	read_level(game, level);

	load_game(game);
}