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

v2i _screenSize = { 20, 10 };

v2i _charPos;
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
int getMapElement(level_t* map, int x, int y)
{
	return map->data[(y * map->size.x) + x].type;
}

internal
int getMapElement(level_t* map, v2i pos)
{
	if (clamp(&pos, map->size.x, map->size.y))
		return -1;

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

internal
void saveGame()
{
	file_t* saveGame;
	if (!openFileForWrite("savegame.txt", &saveGame))
		return;

	writeFileComment(saveGame, "Savegame v1.0");
	writeFileKeyValue(saveGame, "POS_X", "%d", _charPos.x);
	writeFileKeyValue(saveGame, "POS_Y", "%d", _charPos.y);

	writeFileComment(saveGame, "DOORS");

	int doorId = 1;
	mapElement_t* p = _currentLevel->data;
	while (p->type != END_OF_MAP)
	{
		if (isDoor(p))
		{
			writeFileKeyValue(saveGame, "DOOR", "%d %s", doorId, (p->type == DOOR) ? "CLOSED" : "OPEN");
			doorId++;
		}

		p++;
	}

	writeFileComment(saveGame, "END_DOORS");

	writeFileComment(saveGame, "EOF");
	freeFile(saveGame);
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
level_t* readMap(const char* filename) 
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return NULL;

	void* memory = allocate(sizeof(level_t) + (file->size + 1) * sizeof(mapElement_t));
	_currentLevel = (level_t*)memory;
	_currentLevel->data = (mapElement_t*)(((char* )memory) + sizeof(level_t));
	
	char* line;
	mapElement_t* ptr = _currentLevel->data;
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
				_charPos.y = _currentLevel->size.y;
				break;
			}

			line++;
			xOffset++;
			ptr++;
		}

		_currentLevel->size.y++;
		if (xOffset > _currentLevel->size.x)
			_currentLevel->size.x = xOffset;
	}

	ptr->type = END_OF_MAP;

	freeFile(file);
	return _currentLevel;
}

void
init_game()
{
	_charPos = { 1, 1 };
	_currentLevel = readMap("map01.txt");
	loadSaveGame();
}