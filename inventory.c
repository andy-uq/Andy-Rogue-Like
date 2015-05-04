#include <string.h>
#include <stdlib.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

item_t* find_item(collection_t* items, int id)
{
	foreach(item_t*, item, items)
	{
		if (item->id == id)
		{
			return item;
		}
	}

	return 0;
}

void set_item_property(const char* key, const char* value, item_t* item)
{
	if (str_equals("NAME", key))
	{
		item->name = string_alloc(value);
	}
	else if (str_equals("GLYPH", key))
	{
		item->glyph = *value;
	}
	else if (str_equals("ID", key))
	{
		item->id = atoi(value);
	}
}

void load_items(file_t* file, collection_t* items)
{
	char* buffer;
	char* key;
	char* value;

	item_t* item = (item_t*)collection_new_item(items, sizeof(item_t));
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_ITEM"))
		{
			item = (item_t*)collection_new_item(items, sizeof(item_t));
			continue;
		}

		parse_key_value(buffer, &key, &value);
		set_item_property(key, value, item);
	}
}