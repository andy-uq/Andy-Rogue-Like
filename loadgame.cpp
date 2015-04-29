#include <string.h>
#include <stdlib.h>
#include <functional>

#include "arl.h"
#include "platform.h"

typedef int(*ParseKey)(file_t* file, gameState_t* game);
typedef int(*ParseGameKey)(const char* key, const char* valueBuffer, gameState_t* game);
typedef int(*ParsePlayerKey)(const char* key, const char* valueBuffer, player_t* player);

internal int apply(const char* key, const char* buffer, const ParseGameKey* parseFuncs, gameState_t* game)
{
	while (*parseFuncs)
	{
		int result = (*parseFuncs)(key, buffer, game);
		if (result)
			return 1;

		parseFuncs++;
	}

	return 0;
}

internal int apply(const char* key, const char* buffer, const ParsePlayerKey* parseFuncs, player_t* player)
{
	while (*parseFuncs)
	{
		int result = (*parseFuncs)(key, buffer, player);
		if (result)
			return 1;

		parseFuncs++;
	}

	return 0;
}

int parseIntValue(const char* target, const char* key, const char* value, std::function<void(int)> applyFunc)
{
	if (_stricmp(target, key))
		return 0;

	applyFunc(atoi(value));
	return 1;
}

internal
int monster(const char* key, const char* value, gameState_t* game)
{
	if (_stricmp("monster", key) || !game->currentLevel.mobs)
		return 0;

	int targetId = atoi(value);
	int monsterId = 1;
	foreach(monster_t*, m, game->currentLevel.mobs)
	{
		if (monsterId == targetId)
		{
			m->position.x = nextInt(&value);
			m->position.y = nextInt(&value);
			m->energy = nextInt(&value);
			return 1;
		}

		monsterId++;
	}

	return 1;
}

internal
int door(const char* key, const char* value, gameState_t* game)
{
	if (_stricmp("door", key))
		return 0;

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

				return 1;
			}
		}

		p++;
	}

	return 0;
}

int position(const char* key, const char* value, player_t* player)
{
	if (_stricmp(key, "POSITION"))
		return 0;

	player->position.x = atoi(value);
	player->position.y = nextInt(&value);
	return 1;
}

internal
int hp(const char* key, const char* value, player_t* player)
{
	return parseIntValue("HP", key, value, [player](int i) { player->hp = i; });
}

internal
int attack(const char* key, const char* value, player_t* player)
{
	return parseIntValue("ATTACK", key, value, [player](int i) { player->attack = i; });
}

internal
int defense(const char* key, const char* value, player_t* player)
{
	return parseIntValue("DEFENSE", key, value, [player](int i) { player->defense = i; });
}

internal
int damage(const char* key, const char* value, player_t* player)
{
	return parseIntValue("DAMAGE", key, value, [player](int i) { player->damage = i; });
}

ParsePlayerKey _parsePlayerFuncs[] = {
	position,
	hp,
	attack,
	defense,
	damage,
	0
};

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

		parseKeyValue(buffer, [player](const char* key, const char* value) { return apply(key, value, _parsePlayerFuncs, player); });
	}
}

void beginParse(file_t* file, char* buffer, gameState_t* game)
{
	if (_stricmp(buffer, "PLAYER") == 0)
	{
		loadPlayer(file, &game->player);
		return;
	}

	if (str_startswith(buffer, "MONSTER"))
	{
		const char* value = parseValue(buffer);
		int monsterId = atoi(value);

		auto monster = (monster_t* )getAt(game->currentLevel.mobs, monsterId - 1);
		loadMonster(file, monster);
		return;
	}
	
	ParseGameKey parseGameKeys[] = {
		door,
		monster,
		0
	};

	ParseGameKey* pParseGameKeys = parseGameKeys;
	parseKeyValue(buffer, [game, pParseGameKeys](const char* key, const char* value) { return apply(key, value, pParseGameKeys, game); });
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