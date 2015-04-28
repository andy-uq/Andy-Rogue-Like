#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>
#include <tchar.h>

#include "arl.h"
#include "platform.h"

HANDLE _hStdin, _hStdout, _hActiveBuffer, _hBackBuffer;
int _fdwSaveOldMode;
bool _alive;

struct {
	int totalSize, transientSize, stringSize;
	byte* base;
	byte* transient;
	byte* string;
} _memory { 1 << 20, 256 << 10, 256 << 10 };

struct {
	byte* head;
	size_t available;
} _alloced, _transient, _string;

memoryArena_t* freeList;
memoryArena_t arenaStore;

struct win32_file_t
{
	file_t fileInfo;
	FILE* fs;
	char* buffer;
};

#define READBUFFERSIZE (8 * (1 << 10))
#define WRITEBUFFERSIZE (8 * (1 << 10))

void debug(wchar_t* outputString)
{
	OutputDebugStringW(outputString);
	OutputDebugStringW(_T("\n"));
}

void debugf(const wchar_t* format, ...)
{
	va_list argp; 
	va_start(argp, format);
	wchar_t buffer[512];
	vswprintf_s(buffer, 512, format, argp);
	OutputDebugStringW(buffer);
	OutputDebugStringW(_T("\n"));
	va_end(argp);
}

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

internal
int win32_init()
{
	// Get a handle to the STDOUT screen buffer to copy from and 
	// create a new screen buffer to copy to. 

	_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	_hBackBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,				// read/write access
		FILE_SHARE_READ | FILE_SHARE_WRITE,			// shared 
		NULL,										// default security attributes 
		CONSOLE_TEXTMODE_BUFFER,					// must be TEXTMODE 
		NULL										// reserved; must be NULL 
		);

	_hActiveBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,				// read/write access
		FILE_SHARE_READ | FILE_SHARE_WRITE, 		// shared 
		NULL, 										// default security attributes 
		CONSOLE_TEXTMODE_BUFFER, 					// must be TEXTMODE 
		NULL 										// reserved; must be NULL 
		);

	if (_hStdout == INVALID_HANDLE_VALUE || _hActiveBuffer == INVALID_HANDLE_VALUE || _hBackBuffer == INVALID_HANDLE_VALUE)
	{
		debugf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
		return 1;
	}

	// Make the new screen buffer the active screen buffer. 

	if (!SetConsoleActiveScreenBuffer(_hActiveBuffer))
	{
		debugf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
		return 1;
	}

	_hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (_hStdin == INVALID_HANDLE_VALUE)
	{
		debugf("GetStdHandle failed - (%d)\n", GetLastError());
		return 1;
	}

	int fdwMode = 0;
	if (!SetConsoleMode(_hStdin, fdwMode))
	{
		debugf("SetConsoleMode failed - (%d)\n", GetLastError());
		return 1;
	}

	CONSOLE_CURSOR_INFO cursorInfo = {};
	cursorInfo.bVisible = false;
	cursorInfo.dwSize = 100;

	SetConsoleCursorInfo(_hActiveBuffer, &cursorInfo);
	SetConsoleCursorInfo(_hBackBuffer, &cursorInfo);
	return 0;
}

internal
game_input* KeyEventProc(KEY_EVENT_RECORD ker, game_input* input)
{
	if (ker.bKeyDown)
		return input;

	if (ker.uChar.AsciiChar >= '0' && ker.uChar.AsciiChar <= 'z')
	{
		debugf("key %s '%c'", (ker.bKeyDown ? "pressed" : "released"), ker.uChar.AsciiChar);
	}
	else
	{
		debugf("key %s '0x%x'", (ker.bKeyDown ? "pressed" : "released"), ker.wVirtualKeyCode);
	}

	switch (ker.wVirtualKeyCode)
	{
		case 'Q':
			if (!ker.bKeyDown)
			{
				_alive = false;
				input->quit = true;
			}
			break;

		case VK_DOWN:
			input->yOffset = +1;
			break;

		case VK_UP:
			input->yOffset = -1;
			break;

		case VK_LEFT:
			input->xOffset = -1;
			break;

		case VK_RIGHT:
			input->xOffset = +1;
			break;
	}

	return input;
}

internal
game_input readInput()
{
	INPUT_RECORD irInBuf[128];
	DWORD cNumRead;

	ReadConsoleInput(
		_hStdin,      // input buffer handle 
		irInBuf,     // buffer to read into 
		128,         // size of read buffer 
		&cNumRead);

	game_input input = {};
	for (DWORD i = 0; i < cNumRead; i++)
	{
		switch (irInBuf[i].EventType)
		{
		case KEY_EVENT: // keyboard input 
			KeyEventProc(irInBuf[i].Event.KeyEvent, &input);
			break;
		}
	}

	return input;
}

internal
int beginRender()
{
	DWORD write;
	FillConsoleOutputCharacter(_hBackBuffer, _T(' '), 80 * 25, {}, &write);
	return 0;
}

internal
int renderComplete()
{
	HANDLE hPreviousBuffer = _hActiveBuffer;
	_hActiveBuffer = _hBackBuffer;
	
	SetConsoleActiveScreenBuffer(_hActiveBuffer);
	_hBackBuffer = hPreviousBuffer;

	return 0;
}

internal
int win32_close()
{
	SetConsoleMode(_hStdin, _fdwSaveOldMode);	
	return SetConsoleActiveScreenBuffer(_hStdout);
}

void 
drawToBuffer(const char *text)
{
	if (!text)
		return;

	COORD coordBufCoord = {}, coordBufSize = { 80, 2 };
	SMALL_RECT srctWriteRect = { 0, 0, 80, 2 };

	CHAR_INFO textBuffer[160]; 
	memset(textBuffer, 0, sizeof(textBuffer));

	for (int i = 0; i < 160 && *text; i++, text++)
	{
		textBuffer[i].Char.AsciiChar = *text;
		textBuffer[i].Attributes = 0x0f;
	}

	// Copy from the temporary textBuffer to the new screen textBuffer. 
	WriteConsoleOutput(
		_hBackBuffer,		// screen textBuffer to write to 
		textBuffer,			// textBuffer to copy from 
		coordBufSize,		// col-row size of chiBuffer 
		coordBufCoord,		// top left src cell in chiBuffer 
		&srctWriteRect);	// dest. screen textBuffer rectangle 
}

void 
drawLineAt(v2i pos, const char *text)
{
	if (!text)
		return;

	CHAR_INFO textBuffer[80]; 
	memset(textBuffer, 0, sizeof(textBuffer));

	short length;
	for (length = 0; length < 80 && *text; length++, text++)
	{
		textBuffer[length].Char.AsciiChar = *text;
		textBuffer[length].Attributes = 0x0f;
	}

	COORD coordBufCoord = {}, coordBufSize = { length, 1 };
	SMALL_RECT srctWriteRect = { (short)pos.x, (short)pos.y, (short)pos.x + length, (short)pos.y + 1 };

	// Copy from the temporary textBuffer to the new screen textBuffer. 
	WriteConsoleOutput(
		_hBackBuffer,		// screen textBuffer to write to 
		textBuffer,			// textBuffer to copy from 
		coordBufSize,		// col-row size of chiBuffer 
		coordBufCoord,		// top left src cell in chiBuffer 
		&srctWriteRect);	// dest. screen textBuffer rectangle 
}

void
drawfLineAt(v2i pos, const char *format, ...)
{
	char buffer[80];
	va_list argp;
	va_start(argp, format);
	vsprintf_s(buffer, 80, format, argp);
	drawLineAt(pos, buffer);
	va_end(argp);
}

void
drawCharAt(const v2i pos, char c)
{
	CHAR_INFO chiBuffer = {};
	chiBuffer.Attributes = 0x07;
	chiBuffer.Char.AsciiChar = c;

	SMALL_RECT writeRect = { (short)pos.x, (short)pos.y, (short)pos.x + 1, (short)pos.y + 1 };
	WriteConsoleOutput(
		_hBackBuffer,
		&chiBuffer,
		{ 1, 1 },
		{},
		&writeRect);
}

void*
allocate(size_t size)
{
	if (_alloced.available < size)
		return 0;

	byte* alloc = _alloced.head;
	_alloced.head = _alloced.head + size;
	_alloced.available -= size;

	return alloc;
}

void*
allocateTransient(size_t size)
{
	byte* alloc = _transient.head;
	_transient.head = _transient.head + size;
	return alloc;
}

void
resetTransient()
{
	_transient.head = _memory.transient;
	_transient.available = _memory.transientSize;
}

char*
alloc_s(const char* string)
{
	char* p = (char* )_string.head;
	char* result = p;

	while (*string)
	{
		(*p) = *string;
		string++;
	}

	*p = 0;
	_string.head = p + 1;

	return result;
}


internal
void initArena()
{
	arenaStore.size = sizeof(memoryArena_t) * 100;
	arenaStore.head = (byte*)allocate(arenaStore.size);
	arenaStore.tail = arenaStore.head + arenaStore.size;
}

memoryArena_t*
allocateArena(size_t size)
{
	if (freeList) {
		memoryArena_t* best = 0;
		for (auto p = freeList; p; p = p->next) {
			if (p->size > size)
			{
				if (!best || best->size > p->size) {
					best = p;
				}
			}
		}

		if (best)
			return best;
	}

	auto pArenaStore = &arenaStore;
	memoryArena_t* arena = (memoryArena_t*)arenaAlloc(&pArenaStore, sizeof(memoryArena_t));
	if (pArenaStore != &arenaStore)
		arenaStore = *pArenaStore;

	arena->size = size;
	arena->head = (byte*)allocate(size);
	arena->tail = arena->head + arenaStore.size;

	return arena;
}

void*
arenaAlloc(memoryArena_t** pArena, size_t size)
{
	auto arena = *pArena;
	size_t available = arena->tail - arena->head;
	if (available < size)
	{
		int newSize = max(size, arena->size);
		*pArena = (memoryArena_t *)allocateArena(newSize);
		(*pArena)->next = arena;
		arena = *pArena;
	}

	byte* alloc = arena->head;
	arena->head += size;
	return alloc;
}

void
freeArena(memoryArena_t* arena) {
	if (freeList) {
		auto p = freeList;
		while (p->next)
			p++;

		p->next = arena;
	}
	else {
		freeList = arena;
	}

	arena->head = (arena->tail - arena->size);
}

int
openFileForWrite(const char* filename, file_t** file)
{
	FILE* fs;
	if (fopen_s(&fs, filename, "wb"))
		return 0;

	win32_file_t* win32_file = (win32_file_t* )allocateTransient(sizeof(win32_file_t));
	win32_file->fileInfo.size = 0;
	win32_file->fs = fs;
	win32_file->buffer = ((char*)win32_file) + sizeof(win32_file_t);

	(*file) = (file_t*)win32_file;
	return 1;
}

int
openFileForRead(const char* filename, file_t** file)
{
	FILE* fs;
	if (fopen_s(&fs, filename, "rb"))
		return 0;

	struct stat st;
	stat(filename, &st);
	long size = st.st_size;

	win32_file_t* win32_file = (win32_file_t*)allocateTransient(sizeof(win32_file_t) + READBUFFERSIZE);
	(*win32_file) = {};
	win32_file->fileInfo.size = size;
	win32_file->fs = fs;
	win32_file->buffer = ((char*)win32_file) + sizeof(win32_file_t);
	
	(*file) = (file_t* )win32_file;
	return 1;
}

void
writeLine(const file_t* file, const char* line)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	int bytes = fputs(line, win32_file->fs);
	win32_file->fileInfo.size += bytes;
}

char* 
readLine(const file_t* file)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	return fgets(win32_file->buffer, READBUFFERSIZE, win32_file->fs);
}

long
seek(const file_t* file, long offset)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	if (offset < 0)
	{
		return fseek(win32_file->fs, offset, SEEK_END);
	}
	
	return fseek(win32_file->fs, offset, SEEK_SET);
}

void 
freeFile(const file_t* file) 
{
	if (file)
	{
		win32_file_t* win32_file = (win32_file_t*)file;
		fclose(win32_file->fs);
	}
}

int 
_tmain()
{
	if (win32_init())
		return 1;

	LPVOID BaseAddress = (LPVOID)(2LL<<40);

	_memory.base = (byte* )VirtualAlloc(BaseAddress, _memory.totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	_memory.string = (void*)((char*)_memory.base + (_memory.totalSize - _memory.transientSize - _memory.stringSize));
	_alloced.head = _memory.base;
	_alloced.available = (_memory.totalSize - _memory.transientSize);
	
	_memory.transient = _memory.base + (_memory.totalSize - _memory.transientSize);
	resetTransient();

	initArena();
	initGame();

	_alive = true;
	while (_alive)
	{
		resetTransient();
		beginRender();
		updateAndRender();
		renderComplete();

		game_input input = readInput();
		processInput(input);
	}

	win32_close();
	return 0;
}