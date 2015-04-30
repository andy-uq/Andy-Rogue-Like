#include <string.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

internal
void setItemProperty(const char* key, const char* value, item_t* item)
{
	if (str_equals("NAME", key))
	{
		item->name = str_alloc(value);
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
	while ((buffer = readLine(file)) != NULL)
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
		parseKeyValue(buffer, &key, &value);
		setItemProperty(key, value, item);
	}
}

void loadItems(file_t* file, collection_t* items)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		item_t* i = (item_t*)newItem(items, sizeof(item_t));
		*i = (item_t ){ '$' };
		loadItem(file, i);
	}
}