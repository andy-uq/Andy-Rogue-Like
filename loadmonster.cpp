#include "arl.h"
#include "platform.h"

typedef int(*ParseKey)(const char* key, const char* valueBuffer, monster_t* monster);

internal
int apply(const char* key, const char* buffer, const ParseKey* parseFuncs, monster_t* monster)
{
	while (*parseFuncs)
	{
		int result = (*parseFuncs)(key, buffer, monster);
		if (result)
			return 1;

		parseFuncs++;
	}

	return 0;
}

internal
int parseIntValue(const char* target, const char* key, const char* value, std::function<void(int)> applyFunc)
{
	if (_stricmp(target, key))
		return 0;

	applyFunc(atoi(value));
	return 1;
}

internal
int posX(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("POS_X", key, value, [monster](int i) { monster->position.x = i; });
}

internal
int posY(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("POS_Y", key, value, [monster](int i) { monster->position.y = i; });
}

internal
int speed(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("SPEED", key, value, [monster](int i) { monster->speed = i; });
}

internal
int glyph(const char* key, const char* value, monster_t* monster)
{
	if (_stricmp("GLYPH", key))
		return 0;

	monster->glyph = *value;
	return 1;
}

internal
ParseKey _parseFuncs[] = {
	posX,
	posY,
	speed,
	glyph,
	0
};

void loadMonsters(file_t* file, monster_t* m)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);
	
		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_MONSTER"))
		{
			m++;
			*m = { 'M' };
			continue;
		}
		
		parseKeyValue(buffer, [m](const char* key, const char* value) { return apply(key, value, _parseFuncs, m); });
	}

	m->glyph = 0;
}