#include <string.h>
#include <stdlib.h>
#include <functional>

#include "arl.h"
#include "platform.h"

typedef int(*ParseKey)(const char* key, const char* valueBuffer, gameState_t* game);

internal
int nextInt(const char** value)
{
	const char* p = *value;
	while (*p >= '0' && *p <= '9')
		p++;

	while (*p == ' ' || *p == '\t')
		p++;

	*value = p;
	return atoi(p);
}

internal
int parseKeyValue(char* line, ParseKey* parseFuncs, gameState_t* game)
{
	char buffer[512];

	char* key = line;

	char c;
	while ((c = *line) != 0)
	{
		if (c == '=')
		{
			(*line) = 0;
			line++;
			break;
		}

		line++;
	}

	str_trimstart(&line);
	strncpy_s(buffer, line, 512);

	str_trimend(key);
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
	if (_stricmp("monster", key))
		return 0;

	int targetId = atoi(value);
	int monsterId = 1;
	for (monster_t* m = game->currentLevel.mobs; m->glyph; m++)
	{
		if (monsterId == targetId)
		{
			m->position.x = nextInt(&value);
			m->position.y = nextInt(&value);
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

		parseKeyValue(buffer, parseFuncs, game);
	}

	freeFile(file);
}