#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include "arl.h"
#include "memory.h"

void debug(char* outputString)
{
	OutputDebugStringA(outputString);
	OutputDebugStringA("\n");
}

void debugf(const char* format, ...)
{
	char buffer[512];
	va_list argp;
	va_start(argp, format);
	vsprintf_s(buffer, 512, format, argp);
	OutputDebugStringA(buffer);
	OutputDebugStringA("\n");
	va_end(argp);
}

typedef struct
{
	int id; 
	char name[64];
} test_obj_t;

void _create_collection()
{
  collection_t* collection = create_collection(0, 0);
  assert(collection);
  assert(collection_count(collection) == 0);
}

void _add_item_to_collection()
{
  test_obj_t test_obj = { .name="_add_item_to_collection" };
  collection_t* collection = create_collection(1, sizeof(test_obj_t));
  collection_push(collection, &test_obj);

  assert(collection_count(collection) == 1);
  assert(collection_any(collection));
}

void _get_first_item_from_collection()
{
	assert(collection_first(0) == 0);

	collection_t* collection = create_collection(1, sizeof(test_obj_t));
	assert(collection_first(collection) == 0);

	test_obj_t test_obj = { .name = "_get_first_item_from_collection" };
	collection_push(collection, &test_obj);

	test_obj_t* result = collection_pop(collection);
	assert(_stricmp(result->name, test_obj.name) == 0);

	assert(collection_count(collection) == 0);
}

void _get_n_item_from_collection()
{
	test_obj_t t1 = { .name = "3" };
	test_obj_t t2 = { .name = "2" };
	test_obj_t t3 = { .name = "1" };
	
	collection_t* collection = create_collection(1, sizeof(test_obj_t));
	collection_push(collection, &t1);
	collection_push(collection, &t2);
	collection_push(collection, &t3);

	test_obj_t* r1 = collection_get_at(collection, 0);
	test_obj_t* r2 = collection_get_at(collection, 1);
	test_obj_t* r3 = collection_get_at(collection, 2);
	assert(*r1->name == '1');
	assert(*r2->name == '2');
	assert(*r3->name == '3');

	test_obj_t* n1 = collection_next(collection, r1);
	test_obj_t* p1 = collection_prev(collection, r1);
	assert(*n1->name == '2');
	assert(p1 == 0);

	test_obj_t* n2 = collection_next(collection, r2);
	test_obj_t* p2 = collection_prev(collection, r2);
	assert(*n2->name == '3');
	assert(*p2->name == '1');

	test_obj_t* n3 = collection_next(collection, r3);
	test_obj_t* p3 = collection_prev(collection, r3);
	assert(n3 == 0);
	assert(*p3->name == '2');
}

void _remove_item_from_collection()
{
	test_obj_t test_obj = { .name = "_remove_item_from_collection" };
	collection_t* collection = create_collection(1, sizeof(test_obj_t));
	collection_push(collection, &test_obj);
	test_obj_t* result = collection_pop(collection);
	
	assert(_stricmp(result->name, test_obj.name) == 0);

	assert(collection_count(collection) == 0);
	assert(collection_any(collection) == false);
}

void test_collections()
{
	_create_collection();
	_add_item_to_collection();
	_get_first_item_from_collection();
	_get_n_item_from_collection();
	_remove_item_from_collection();
}

void test_hashtable()
{
	hashtable_t* hashtable = create_int_hashtable(10000);
	assert(hashtable);

	test_obj_t i1 = { .name = "_add_item_to_collection" };
	hashtable_add(hashtable, &i1.id, &i1);
	assert(hashtable_contains(hashtable, &i1.id));
	
	int id = 0;
	test_obj_t* result = hashtable_get(hashtable, &id);
	assert(result);
	
	int missing_id = 1;
	result = hashtable_get(hashtable, &missing_id);
	assert(result == 0);

	test_obj_t i2 = { .id = 1000, .name = "_add_item_to_collection" };
	hashtable_add(hashtable, &i2.id, &i2);
	assert(hashtable_contains(hashtable, &i2.id));

	hashtable_remove(hashtable, &i1.id);
	assert(hashtable_contains(hashtable, &i1.id) == false);
	assert(hashtable_contains(hashtable, &i2.id));

	hashtable_clear(hashtable);
	assert(hashtable_contains(hashtable, &i1.id) == false);
	assert(hashtable_contains(hashtable, &i2.id) == false);

	test_obj_t* buffer = malloc(sizeof(test_obj_t) * 5000);
	test_obj_t* p = buffer;
	for (int i = 0; i < 5000; i++, p++)
	{
		assert(hashtable_add(hashtable, &i, p));
	}

	for (int i = 0; i < 5000; i++)
	{
		assert(hashtable_contains(hashtable, &i));
	}

	hashtable_resize(hashtable, 10000);
	for (int i = 0; i < 5000; i++)
	{
		assert(hashtable_contains(hashtable, &i));
	}
}

internal
void _str_equals()
{
	assert(str_equals("hello", "hello"));
	assert(str_equals("hello", "HELLO"));
	assert(str_equals("hello", "world") == false);
}

internal
void _str_startswith()
{
	assert(str_startswith("hello", "hell"));
	assert(str_startswith("hello", "hell"));
	assert(str_startswith("hello", "HELL"));
	assert(str_startswith("hello", ""));
}

internal
void _str_endswith()
{
	assert(str_endswith("hello", "llo"));
	assert(str_endswith("hello", ""));
	assert(str_endswith("hello", "world") == false);
}

void test_strings()
{
	_str_equals();
	_str_startswith();
	_str_endswith();
}

void main() {

  LPVOID BaseAddress = (LPVOID)0x2000000;
  byte* base = (byte* )VirtualAlloc(BaseAddress, GAME_HEAP_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  memory_init(base, GAME_HEAP_SIZE);

  test_hashtable();
  test_collections();
  test_strings();

  printf("Tests were all good!\n");
  exit(0);
}
