#include <string.h>
#include <stdlib.h>
#include <functional>

#include "arl.h"
#include "platform.h"

typedef int(*ParseKey)(const char* key, const char* valueBuffer, gameState_t* game);

internal int apply(const char* key, const char* buffer, const ParseKey* parseFuncs, gameState_t* game)
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

int parseIntValue(const char* target, const char* key, const char* value, std::function<void(int)> applyFunc)
{
	if (_stricmp(target, key))
		return 0;

	applyFunc(atoi(value));
	return 1;
}

int posX(const char* key, const char* value, gameState_t* game)
{
	return parseIntValue("pos_x", key, value, [game](int i) { game->charPos.x = i; });
}

int posY(const char* key, const char* value, gameState_t* game)
{
	return parseIntValue("pos_y", key, value, [game](int i) { game->charPos.y = i; });
}

internal
int monster(const char* key, const char* value, gameState_t* game)
{
	if (_stricmp("monster", key) || !game->currentLevel.mobs)
		return 0;

	int targetId = atoi(value);
	int monsterId = 1;
	for (monster_t* m = game->currentLevel.mobs; m->glyph; m++)
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

void loadGame(gameState_t* game)
{
	file_t* file;
	if (!openFileForRead("savegame.txt", &file))
		return;

	const char* line;
	char buffer_[512];
	char* buffer = buffer_;

	ParseKey parseFuncs[] = {
		posX,
		posY,
		door,
		monster,
		0
	};

	while ((line = readLine(file)) != NULL)
	{
		strncpy_s(buffer, 512, line, _TRUNCATE);
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		const ParseKey* p = parseFuncs;
		parseKeyValue(buffer, [=](const char* key, const char* value) { return apply(key, value, p, game); });
	}

	freeFile(file);
}