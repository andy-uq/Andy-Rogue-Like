#include "arl.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

enum elementType_t {
	FLOOR,
	WALL,
	DOOR,
	OPEN_DOOR,


	END_OF_MAP
};

struct mapElement_t {
	elementType_t type;
};

struct level_t
{
	const char* filename;
	mapElement_t* data;
	v2i size;
};

struct monster_t
{
	char glyph;
	v2i position;
};

v2i _screenSize = { 20, 10 };

v2i _charPos;
monster_t* _mobs;

level_t* _currentLevel;

internal
v2i toScreenCoord(v2i pos)
{
	int xOffset = (80 - _screenSize.x) / 2;
	int yOffset = (25 - _screenSize.y) / 2;

	return { pos.x + xOffset, pos.y + yOffset };
}

internal
bool clamp(v2i* pos, int x, int y)
{
	bool clamped = false;

	if (pos->x < 0)
	{
		pos->x = 0;
		clamped = true;
	}

	if (pos->y < 0)
	{
		pos->y = 0;
		clamped = true;
	}

	if (pos->y > y)
	{
		pos->y = y;
		clamped = true;
	}

	if (pos->x > x)
	{
		pos->x = x;
		clamped = true;
	}

	return clamped;
}

internal
elementType_t getMapElement(level_t* map, int x, int y)
{
	return map->data[(y * map->size.x) + x].type;
}

internal
elementType_t getMapElement(level_t* map, v2i pos)
{
	if (clamp(&pos, map->size.x, map->size.y))
		return (elementType_t )-1;

	return getMapElement(map, pos.x, pos.y);
}

internal void
renderMap(level_t* map)
{
	v2i mapOffset = { _charPos.x - (_screenSize.x / 2), _charPos.y - (_screenSize.y / 2) };
	clamp(&mapOffset, map->size.x - _screenSize.x, map->size.y - _screenSize.y);

	v2i p = {};
	for (p.x = 0; p.x < _screenSize.x; p.x++)
		for (p.y = 0; p.y < _screenSize.y; p.y++)
		{
			int x = p.x + mapOffset.x;
			int y = p.y + mapOffset.y;

			char c;
			if (_charPos.x == x && _charPos.y == y)
			{
				c = '@';
			}
			else
			{
				int mapElement = getMapElement(map, x, y);
				switch (mapElement)
				{
				case FLOOR:
					c = '.';
					break;
				case DOOR:
					c = '/';
					break;
				case OPEN_DOOR:
					c = '=';
					break;
				case WALL:
				default:
					c = '#';
					break;
				}
			}

			drawCharAt(toScreenCoord(p), c);
		}

	for (monster_t* m = _mobs; m->glyph; m++)
	{
		v2i xy = { m->position.x - mapOffset.x, m->position.y - mapOffset.y };
		if (clamp(&xy, _screenSize.x, _screenSize.y))
			continue;

		drawCharAt(toScreenCoord(xy), m->glyph);
	}
}

void
updateAndRender()
{
	renderMap(_currentLevel);
}

internal
int openDoor(level_t* map, v2i pos)
{
	if (clamp(&pos, map->size.x, map->size.y))
		return 0;

	map->data[(pos.y * map->size.x) + pos.x].type = OPEN_DOOR;
	return 1;
}

internal
char* append(char* dest, const char* source)
{
	while (*source)
	{
		*(dest++) = *source;
		source++;
	}

	return dest;
}

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
	
	write = append(write, comment);

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

	char* write = append(line, key);
	*(write++) = '=';
	write = append(write, value);
	*(write++) = '\r';
	*(write++) = '\n';
	*(write++) = 0;

	writeLine(file, line);
}

internal
bool isDoor(mapElement_t* e)
{
	return
		e->type == DOOR
		|| e->type == OPEN_DOOR;
}

internal void saveDoors(file_t* saveGame)
{
	writeFileComment(saveGame, "DOORS");

	int doorId = 1;
	for (mapElement_t* p = _currentLevel->data; p->type != END_OF_MAP; p++)
	{
		if (isDoor(p))
		{
			writeFileKeyValue(saveGame, "DOOR", "%d %s", doorId, (p->type == DOOR) ? "CLOSED" : "OPEN");
			doorId++;
		}
	}

	writeFileComment(saveGame, "END_DOORS");
}

internal void saveMonsters(file_t* saveGame)
{
	writeFileComment(saveGame, "MONSTERS");

	int monsterId = 1;
	for (monster_t* m = _mobs; m->glyph; m++)
	{
		writeFileKeyValue(saveGame, "MONSTER", "%d %d %d", monsterId, m->position.x, m->position.y);
		monsterId++;
	}

	writeFileComment(saveGame, "MONSTERS");
}

internal
void saveGame()
{
	file_t* saveGame;
	if (!openFileForWrite("savegame.txt", &saveGame))
		return;

	writeFileComment(saveGame, "Savegame v1.0");
	writeFileKeyValue(saveGame, "POS_X", "%d", _charPos.x);
	writeFileKeyValue(saveGame, "POS_Y", "%d", _charPos.y);

	saveDoors(saveGame);
	saveMonsters(saveGame);

	writeFileComment(saveGame, "EOF");
	freeFile(saveGame);
}

int sRandom = 1337;

internal
void moveMob(v2i* m)
{
	v2i newPos = *m;

	sRandom *= 113;

	if (sRandom % 11 == 0) newPos.y--;
	else if (sRandom % 7 == 1) newPos.y++;
	else if (sRandom % 5 == 2) newPos.x--;
	else if (sRandom % 3 == 3) newPos.x++;

	if (clamp(&newPos, _currentLevel->size.x, _currentLevel->size.y))
		return;

	elementType_t mapElement = getMapElement(_currentLevel, newPos);
	if (mapElement == OPEN_DOOR || mapElement == FLOOR)
	{
		*m = newPos;
	}
}

internal
void moveMobs()
{
	for (monster_t* m = _mobs; m->glyph; m++)
	{
		moveMob(&m->position);
	}
}

void
processInput(const game_input input)
{
	v2i new_pos = { _charPos.x + input.xOffset, _charPos.y + input.yOffset };
	int mapElement = getMapElement(_currentLevel, new_pos);
	switch (mapElement)
	{
	case 1:
		return;
	case 2:
		openDoor(_currentLevel, new_pos);
		return;
	default:
		_charPos = new_pos;
		break;
	}

	moveMobs();
	saveGame();
}

char* trimSpacesEnd(char* line)
{
	char* p = line;

	while (*(p + 1))
		p++;

	while (*p == ' ' || *p == '\r' || *p == '\n')
	{
		*p = 0;
		p--;
	}

	return line;
}

void trimSpaces(char** line)
{
	const char* p = (*line);
	while (*p && *p == ' ')
		p++;
}

void trim(char** line)
{
	char* p = (*line);
	while (*p && *p == ' ')
		p++;

	while (*(p + 1))
		p++;

	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
	{
		*p = 0;
		p--;
	}
}

typedef int(*ParseKey)(const char* key, const char* valueBuffer);

internal
int parseKeyValue(char* line, ParseKey* parseFuncs)
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

	trimSpaces(&line);
	strncpy_s(buffer, line, 512);

	trimSpacesEnd(key);
	while (*parseFuncs)
	{
		int result = (*parseFuncs)(key, buffer);
		if (result)
			return 1;

		parseFuncs++;
	}

	return 0;
}

int parseIntValue(const char* target, const char* key, const char* value, void(*applyFunc)(int))
{
	if (_stricmp(target, key))
		return 0;

	applyFunc(atoi(value));
	return 1;
}

int posX(const char* key, const char* value)
{
	return parseIntValue("pos_x", key, value, [](int i) { _charPos.x = i; });
}

int posY(const char* key, const char* value)
{
	return parseIntValue("pos_y", key, value, [](int i) { _charPos.y = i; });
}

bool endsWith(const char* target, const char* compareTo)
{
	const char* pTarget = target;
	const char* pCompareTo = compareTo;

	while (*pCompareTo)
	{
		pCompareTo++;
		pTarget++;

		if (*pTarget == 0)
			return false;
	}

	while (*pTarget)
	{
		pTarget++;
	}

	while (pCompareTo != compareTo)
	{
		if (*pTarget != *pCompareTo)
			return false;

		pTarget--;
		pCompareTo--;
	}

	return true;
}

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
int monster(const char* key, const char* value)
{
	if (_stricmp("monster", key))
		return 0;

	int targetId = atoi(value);
	int monsterId = 1;
	for (monster_t* m = _mobs; m->glyph; m++)
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
int door(const char* key, const char* value)
{
	if (_stricmp("door", key))
		return 0;

	int targetDoorId = atoi(value);
	int doorId = 0;
	mapElement_t* p = _currentLevel->data;
	while (p->type != END_OF_MAP)
	{
		if (isDoor(p))
		{
			doorId++;
			if (doorId == targetDoorId)
			{
				p->type = endsWith(value, "OPEN")
					? OPEN_DOOR
					: DOOR;

				return 1;
			}
		}

		p++;
	}
	
	return 0;
}

internal
void loadSaveGame()
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
		trim(&buffer);

		if (buffer[0] == '#')
			continue;

		parseKeyValue(buffer, parseFuncs);
	}

	freeFile(file);
}

internal
int readMap(const char* filename, level_t* level) 
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return 0;

	void* memory = allocate((file->size + 1) * sizeof(mapElement_t));
	level->data = ( mapElement_t* )memory;
	
	mapElement_t* ptr = level->data;

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
				_charPos.x = xOffset;
				_charPos.y = level->size.y;
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
bool parseKey(char* buffer, const char* key, char** value)
{
	while (*buffer)
	{
		if (*key == 0)
		{
			*value = buffer + 1;
			return true;
		}

		if (*buffer != *key)
			break;

		buffer++;
		key++;
	}

	return false;
}

internal
level_t* readLevel(level_t* level)
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
		trim(&buffer);

		if (buffer[0] == '#')
			continue;

		char* value;
		if (parseKey(buffer, "MAP", &value))
		{
			if (!readMap(value, level))
				return NULL;

			continue;
		}
		
		if (parseKey(buffer, "MONSTER", &value))
		{
		}
	}

	freeFile(file);
	return level;
}

void
init_game()
{
	_charPos = { 1, 1 };
	_mobs = (monster_t *)allocate(sizeof(monster_t)*10);
	_mobs[0] = { 'M', {1, 1} };
	_mobs[1] = { 'M', {37, 17} };
	_mobs[2].glyph = 0;

	static level_t level = { "level01.txt" };
	_currentLevel = readLevel(&level);
	loadSaveGame();
}