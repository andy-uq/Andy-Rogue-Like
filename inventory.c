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

void set_player_item_property(const char* key, const char* value, stacked_item_t* item)
{
	if (str_equals("QUANTITY", key))
	{
		item->quantity=atoi(value);
	}
}

void load_items(file_t* file, arena_t* storage, hashtable_t* items)
{
	char* buffer;
	char* key;
	char* value;

	item_t* item = 0;
	while ((buffer = file_read(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		if (str_startswith(buffer, "END_ITEM"))
		{
			hashtable_add(items, &item->id, item);
			item = 0;
			continue;
		}

		if (!item)
		{
			item = (item_t*)arena_alloc(&storage, sizeof(*item));
		}

		parse_key_value(buffer, &key, &value);
		set_item_property(key, value, item);
	}
}

stacked_item_t* stacked_find(collection_t* collection, int itemid)
{
	foreach(stacked_item_t*, stacked, collection)
	{
		if (stacked->item->id == itemid)
			return stacked;
	}

	return 0;
}

item_t* stacked_remove(collection_t* collection, stacked_item_t* stacked)
{
	stacked->quantity--;
	if (!stacked->quantity)
	{
		collection_remove(collection, stacked);
	}

	return stacked->item;
}

stacked_item_t* stacked_add(collection_t* collection, item_t* item)
{
	stacked_item_t* stacked = stacked_find(collection, item->id);
	if (stacked)
	{
		stacked->quantity++;
	}
	else
	{
		stacked = collection_new_item(collection, sizeof(stacked_item_t));
		stacked->item = item;
		stacked->quantity = 1;
	}

	return stacked;
}