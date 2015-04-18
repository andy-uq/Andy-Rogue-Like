#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>
#include <tchar.h>

#include "arl.h"

HANDLE _hStdin, _hStdout, _hActiveBuffer, _hBackBuffer;
int _fdwSaveOldMode;
bool _alive;

struct {
	int totalSize, transientSize;
	void* base;
	void* transient;
} _memory { 1 << 20, 256 << 10 };

struct {
	void* head;
} _alloced;


struct win32_file_t
{
	file_t fileInfo;
	FILE* fs;
	char* readBuffer;
};

#define READBUFFERSIZE (8 * (1 << 10))

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
	void* alloc = _alloced.head;
	_alloced.head = (char* )_alloced.head + size;
	return alloc;
}

int
readFile(const char* filename, file_t** file)
{
	FILE* fs;
	if (fopen_s(&fs, filename, "rb"))
		return 0;

	struct stat st;
	stat(filename, &st);
	long size = st.st_size;

	win32_file_t* win32_file = (win32_file_t* )VirtualAlloc(0, sizeof(win32_file_t) +  READBUFFERSIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	win32_file->fileInfo.size = size;
	win32_file->fs = fs;
	win32_file->readBuffer = ((char*)win32_file) + sizeof(win32_file_t);
	
	(*file) = (file_t* )win32_file;
	return 1;
}

char* 
readLine(file_t* file)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	return fgets(win32_file->readBuffer, READBUFFERSIZE, win32_file->fs);
}

void 
freeFile(file_t* file) 
{
	if (file)
	{
		win32_file_t* win32_file = (win32_file_t*)file;
		fclose(win32_file->fs);

		VirtualFree(file, 0, MEM_RELEASE);
	}
}

int 
_tmain()
{
	if (win32_init())
		return 1;

	LPVOID BaseAddress = (LPVOID)(2LL<<40);
	_memory.base = VirtualAlloc(BaseAddress, _memory.totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	_memory.transient = (void* )((char* )_memory.base + (_memory.totalSize - _memory.transientSize));
	_alloced.head = _memory.base;

	init_game();

	_alive = true;
	while (_alive)
	{
		beginRender();
		updateAndRender();
		renderComplete();

		game_input input = readInput();
		processInput(input);
	}

	win32_close();
	return 0;
}