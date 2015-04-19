#include "arl.h"

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
