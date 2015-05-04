#include <string.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

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
}

internal
void loadItem(file_t* file, item_t* item)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_ITEM"))
		{
			return;
		}

		char* key;
		char* value;
		parse_key_value(buffer, &key, &value);
		set_item_property(key, value, item);
	}
}

void load_items(file_t* file, collection_t* items)
{
	char* buffer;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		item_t* i = (item_t*)collection_new_item(items, sizeof(item_t));
		*i = (item_t ){ '$' };
		loadItem(file, i);
	}
}