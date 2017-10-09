#include <stdlib.h>
#include <string.h>

#include "arl.h"
#include "platform.h"

internal 
setIntRange(const char* value, int_range_t* range)
{
	char* context;
	char* token;

	token = strtok_s(value, "-", &context);
	range->min = atoi(token);

	token = strtok_s(NULL, "-", &context);
	range->max = atoi(token);
}

internal
void setMonsterProperty(const char* key, const char* value, monster_info_t* monster, stringtable_t* names)
{
	if (str_equals("NAME", key))
	{
		monster->name = stringtable_add(&names, value);
	}
	else if (str_equals("DESCRIPTION", key))
	{
		monster->name = stringtable_add(&names, value);
	}
	else if (str_equals("SPEED", key))
	{
		setIntRange(&monster->speed, value);
	}
	else if (str_equals("ENERGY", key))
	{
		setIntRange(&monster->energy, value);
	}
	else if (str_equals("DAMAGE", key))
	{
		setIntRange(&monster->damage, value);
	}
	else if (str_equals("DEFENSE", key))
	{
		setIntRange(&monster->defense, value);
	}
	else if (str_equals("ATTACK", key))
	{
		setIntRange(&monster->attack, value);
	}
	else if (str_equals("HP", key))
	{
		setIntRange(&monster->hp, value);
	}
}

void load_monster_info(file_t* file, monster_t* monster)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_MONSTER_INFO"))
		{
			return;
		}

		char* key;
		char* value;
		parse_key_value(buffer, &key, &value);
		setMonsterProperty(key, value, monster);
	}
}

void load_monsters(file_t* file, collection_t* monsters, stringtable_t* names)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);
	
		if (buffer[0] == '#')
			continue;

		monster_info_t* monster = (monster_info_t* )collection_new_item(monsters, sizeof(monster_info_t));
		*monster = { 0 };
		load_monster(file, monster, names);
	}
}