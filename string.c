#include "arl.h"
#include<string.h>

char* str_append(char* dest, const char* source)
{
	while (*source)
	{
		*(dest++) = *source;
		source++;
	}

	return dest;
}

char* str_trimend(char* line)
{
	char* p = line;

	while (*(p + 1))
		p++;

	while (*p == ' ' || *p == '\r' || *p == '\n')
	{
		*p = 0;
		p--;
	}

	return line;
}

void str_trimstart(char** line)
{
	const char* p = (*line);
	while (*p && *p == ' ')
		p++;
}

void str_trim(char** line)
{
	char* p = (*line);
	while (*p && *p == ' ')
		p++;

	while (*(p + 1))
		p++;

	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
	{
		*p = 0;
		p--;
	}
}

boolean str_endswith(const char* target, const char* compareTo)
{
	const char* pTarget = target;
	const char* pCompareTo = compareTo;

	while (*pCompareTo)
	{
		pCompareTo++;
		pTarget++;

		if (*pTarget == 0)
			return false;
	}

	while (*pTarget)
	{
		pTarget++;
	}

	while (pCompareTo != compareTo)
	{
		if (*pTarget != *pCompareTo)
			return false;

		pTarget--;
		pCompareTo--;
	}

	return true;
}

boolean str_startswith(const char* target, const char* compareTo)
{
	const char* pTarget = target;
	const char* pCompareTo = compareTo;

	while (*pCompareTo)
	{
		if (*pTarget != *pCompareTo)
			return false;
		
		if (*pTarget == 0)
			return false;

		pCompareTo++;
		pTarget++;
	}

	return true;
}

boolean str_equals(const char* target, const char* compareTo)
{
	return _stricmp(target, compareTo) == 0;
}
