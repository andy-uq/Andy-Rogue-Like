#include "arl.h"
#include "platform.h"

typedef int(*ParseItemKey)(const char* key, const char* valueBuffer, item_t* monster);

internal
int apply(const char* key, const char* buffer, const ParseItemKey* parseFuncs, item_t* item)
{
	while (*parseFuncs)
	{
		int result = (*parseFuncs)(key, buffer, item);
		if (result)
			return 1;

		parseFuncs++;
	}

	return 0;
}

internal
int glyph(const char* key, const char* value, item_t* item)
{
	if (_stricmp("GLYPH", key))
		return 0;

	item->glyph = *value;
	return 1;
}

internal
int name(const char* key, const char* value, item_t* item)
{
	if (_stricmp("NAME", key))
		return 0;

	item->name = alloc_s(value);
	return 1;
}

internal
ParseItemKey _parseFuncs[] = {
	name,
	glyph,
	0
};

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

		parseKeyValue(buffer, [item](const char* key, const char* value) { return apply(key, value, _parseFuncs, item); });
	}
}

void loadItems(file_t* file, item_t* i)
{
	char* buffer;
	while ((buffer = readLine(file)) != NULL)
	{
		str_trim(&buffer);

		if (buffer[0] == '#')
			continue;

		*i = { '$' };
		loadItem(file, i);

		i++;
	}

	i->glyph = 0;
}