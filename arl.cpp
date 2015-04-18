#include "arl.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>  

enum elementType_t {
	FLOOR,
	WALL,
	DOOR,
	OPEN_DOOR,
};

struct mapElement_t {
	elementType_t type;
};

struct level_t {
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
void saveGame()
{
	file_t* saveGame;
	if (!openFileForWrite("savegame.txt", &saveGame))
		return;

	writeFileComment(saveGame, "Savegame v1.0");
	writeFileKeyValue(saveGame, "POS_X", "%d", _charPos.x);
	writeFileKeyValue(saveGame, "POS_Y", "%d", _charPos.y);
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

internal
level_t* readMap(const char* filename) 
{
	file_t* file;
	if (!openFileForRead(filename, &file))
		return NULL;

	void* memory = allocate(sizeof(level_t) + file->size);
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

	freeFile(file);
	return _currentLevel;
}

void
init_game()
{
	_charPos = { 1, 1 };
	_currentLevel = readMap("map01.txt");
}