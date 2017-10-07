#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "memory.h"
#include "arl.h"

#define MAX_MESSAGE_LENGTH 80
#define MAX_MESSAGES 20

internal
void _messages_remove(messages_t* messages)
{
	messages->head = (messages->head + 1) % MAX_MESSAGES;
}

internal
void _messages_add(messages_t* messages, const char* message) 
{
	int nextInsertPoint = (messages->insertIndex + 1) % MAX_MESSAGES;
	if (nextInsertPoint == messages->head)
		_messages_remove(messages);

	message_t target = messages->list[messages->insertIndex];
	strcpy_s(target.message, MAX_MESSAGE_LENGTH, message);
	messages->insertIndex = nextInsertPoint;
}

/* move through message list, most recent first  */
message_t* iterate_message(const messages_t* messages, message_iterator_t* iterator)
{
	int index;

	if (iterator->index == 0)
	{
		index = messages->insertIndex;
	}
	else 
	{
		index = iterator->index & ~0x8000;
	}

	if (index == messages->head)
		return NULL;

	if (index == 0)
	{
		index = MAX_MESSAGES;
	}

	index--;

	iterator->index = index | 0x8000;
	return &messages->list[index];
}

void messagef(messages_t* messages, const char* format, ...)
{
	va_list argp;
	va_start(argp, format);
	char* message = transient_alloc(MAX_MESSAGE_LENGTH);
	vsprintf_s(message, MAX_MESSAGE_LENGTH, format, argp);
	va_end(argp);

	_messages_add(messages, message);
}

void messages_init(game_t* game) 
{
	arena_t** storage = &game->storage;

	game->messages = (messages_t) { 0 };
	game->messages.list = arena_alloc(storage, MAX_MESSAGES * sizeof(message_t));
	for (int i = 0; i < MAX_MESSAGES; i++) 
	{
		game->messages.list[i].message = arena_alloc(storage, MAX_MESSAGE_LENGTH);
	}
}