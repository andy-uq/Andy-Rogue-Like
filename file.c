#include "arl.h"

internal
char* parseValue(char* line)
{
	while (*line != '=')
	{
		if (*line == 0)
			return line;

		line++;
	}

	(*line) = 0;

	char* value = line + 1;
	str_trim(&value);

	return value;
}

void parseKeyValue(char* line, char** key, char** value)
{
	*key = line;
	*value = parseValue(line);

	str_trim(key);
	str_trim(value);
}

boolean tryGetValueIfKey(char* buffer, const char* key, char** value)
{
	while (*buffer)
	{
		if (*key == 0)
		{
			*value = buffer + 1;
			return true;
		}

		if (*buffer != *key)
			break;

		buffer++;
		key++;
	}

	return false;
}
