#include <stdlib.h>
#include <string.h>

#include "arl.h"
#include "platform.h"

internal
void setMonsterProperty(const char* key, char* value, monster_t* monster)
{
	if (str_equals("GLYPH", key))
	{
		monster->glyph = *value;
	}
	else if (str_equals("SPEED", key))
	{
		monster->speed = atoi(value);
	}
	else if (str_equals("ENERGY", key))
	{
		monster->energy = atoi(value);
	}
	else if (str_equals("DAMAGE", key))
	{
		monster->damage = atoi(value);
	}
	else if (str_equals("DEFENSE", key))
	{
		monster->defense = atoi(value);
	}
	else if (str_equals("ATTACK", key))
	{
		monster->attack = atoi(value);
	}
	else if (str_equals("HP", key))
	{
		monster->hp = atoi(value);
	}
	else if (str_equals("POSITION", key))
	{
		char* context;
		char* token;

		token = strtok_s(value, " ", &context);
		monster->position.x = atoi(token);

		token = strtok_s(NULL, " ", &context);
		monster->position.y = atoi(token);
	}
}

void load_monster(file_t* file, monster_t* monster)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_MONSTER"))
		{
			return;
		}

		char* key;
		char* value;
		parse_key_value(buffer, &key, &value);
		setMonsterProperty(key, value, monster);
	}
}

void load_monsters(file_t* file, collection_t* monsters)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);
	
		if (buffer[0] == '#')
			continue;

		monster_t* monster = (monster_t* )collection_new_item(monsters, sizeof(monster_t));
		*monster = (monster_t ){ 'M' };
		load_monster(file, monster);
	}
}