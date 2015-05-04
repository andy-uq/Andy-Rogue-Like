#include "arl.h"

internal
char* parse_value(char* buffer)
{
	while (*buffer != '=')
	{
		if (*buffer == 0)
			return buffer;

		buffer++;
	}

	(*buffer) = 0;

	char* value = buffer + 1;
	str_trim(&value);

	return value;
}

void parse_key_value(char* buffer, char** key, char** value)
{
	*key = buffer;
	*value = parse_value(buffer);

	str_trim(key);
	str_trim(value);
}

boolean parse_value_if_match(char* buffer, const char* target, char** value)
{
	while (*buffer)
	{
		if (*target == 0)
		{
			*value = buffer + 1;
			return true;
		}

		if (*buffer != *target)
			break;

		buffer++;
		target++;
	}

	return false;
}
