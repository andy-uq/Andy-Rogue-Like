#include "arl.h"
#include "platform.h"

typedef int(*ParseMonsterKey)(const char* key, const char* valueBuffer, monster_t* monster);

internal
int apply(const char* key, const char* buffer, const ParseMonsterKey* parseFuncs, monster_t* monster)
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
int position(const char* key, const char* value, monster_t* monster)
{
	if (_stricmp(key, "POSITION"))
		return 0;

	monster->position.x = atoi(value);
	monster->position.y = nextInt(&value);
	return 1;
}

internal
int hp(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("HP", key, value, [monster](int i) { monster->hp = i; });
}

internal
int attack(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("ATTACK", key, value, [monster](int i) { monster->attack = i; });
}

internal
int defense(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("DEFENSE", key, value, [monster](int i) { monster->defense = i; });
}

internal
int damage(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("DAMAGE", key, value, [monster](int i) { monster->damage = i; });
}

internal
int energy(const char* key, const char* value, monster_t* monster)
{
	return parseIntValue("ENERGY", key, value, [monster](int i) { monster->energy = i; });
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
ParseMonsterKey _parseFuncs[] = {
	position,
	hp,
	attack,
	defense,
	damage,
	energy,
	speed,
	glyph,
	0
};

void loadMonster(file_t* file, monster_t* monster)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_MONSTER"))
		{
			return;
		}

		parseKeyValue(buffer, [monster](const char* key, const char* value) { return apply(key, value, _parseFuncs, monster); });
	}
}

void loadMonsters(file_t* file, monster_t* m)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);
	
		if (buffer[0] == '#')
			continue;

		*m = { 'M' };
		loadMonster(file, m);

		m++;
	}

	m->glyph = 0;
}