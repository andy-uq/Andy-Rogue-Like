#include <functional>
#include "arl.h"

int nextInt(const char** value)
{
	const char* p = *value;
	while (*p >= '0' && *p <= '9')
		p++;

	while (*p == ' ' || *p == '\t')
		p++;

	*value = p;
	return atoi(p);
}

char* parseValue(char* line)
{
	while ((*line != '='))
	{
		if (*line == 0)
			return line;

		line++;
	}

	(*line) = 0;

	auto value = line + 1;
	str_trim(&value);

	return value;
}

int parseKeyValue(char* line, std::function<int(const char*, const char*)> func)
{
	char* key = line;

	char c;
	while ((c = *line) != 0)
	{
		if (c == '=')
		{
			(*line) = 0;
			line++;
			break;
		}

		line++;
	}

	str_trim(&line);
	str_trimend(key);

	return func(key, line);
}

bool parseKey(char* buffer, const char* key, char** value)
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
