#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>
#include <tchar.h>

#include "arl.h"
#include "memory.h"
#include "platform.h"

HANDLE _hStdin, _hStdout, _hActiveBuffer, _hBackBuffer;
int _fdwSaveOldMode;
boolean _alive;

typedef struct
{
	file_t fileInfo;
	FILE* fs;
	char* buffer;
} win32_file_t;

#define READBUFFERSIZE (8 * (1 << 10))
#define WRITEBUFFERSIZE (8 * (1 << 10))

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

	CONSOLE_CURSOR_INFO cursorInfo = { 0 };
	cursorInfo.bVisible = false;
	cursorInfo.dwSize = 100;

	SetConsoleCursorInfo(_hActiveBuffer, &cursorInfo);
	SetConsoleCursorInfo(_hBackBuffer, &cursorInfo);
	return 0;
}

internal
game_input_t* KeyEventProc(KEY_EVENT_RECORD ker, game_input_t* input)
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
		case VK_DOWN:
			input->y_offset = +1;
			break;

		case VK_UP:
			input->y_offset = -1;
			break;

		case VK_LEFT:
			input->x_offset = -1;
			break;

		case VK_RIGHT:
			input->x_offset = +1;
			break;

		case 'Q':
			_alive = false;
			input->action = GAME_ACTION_QUIT;
			break;

		case 'P':
			input->action = GAME_ACTION_PICKUP;
			break;

		case 'D':
			input->action = GAME_ACTION_DROP;
			break;
	}

	return input;
}

internal
game_input_t readInput()
{
	INPUT_RECORD irInBuf[128];
	DWORD cNumRead;

	ReadConsoleInput(
		_hStdin,      // input buffer handle 
		irInBuf,     // buffer to read into 
		128,         // size of read buffer 
		&cNumRead);

	game_input_t input = {0};
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
	FillConsoleOutputCharacter(_hBackBuffer, _T(' '), 80 * 25, (COORD){ 0 }, &write);
	FillConsoleOutputAttribute(_hBackBuffer, 0x0f, 80 * 25, (COORD){ 0 }, &write);
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
draw_to_buffer(const char *text)
{
	if (!text)
		return;

	COORD coordBufCoord = {0}, coordBufSize = { 80, 2 };
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


void makeDark()
{
	DWORD write;
	FillConsoleOutputAttribute(_hBackBuffer, 0x08, 80 * 25, (COORD){ 0 }, &write);
}

void 
draw_line(const v2i pos, const char *text)
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

	COORD coordBufCoord = {0}, coordBufSize = { .X = length, .Y = 1 };
	SMALL_RECT writeRect = { .Left = (short)pos.x, .Top = (short)pos.y, .Right = (short)pos.x + length, .Bottom = (short)pos.y + 1 };

	// Copy from the temporary textBuffer to the new screen textBuffer. 
	WriteConsoleOutput(
		_hBackBuffer,		// screen textBuffer to write to 
		textBuffer,			// textBuffer to copy from 
		coordBufSize,		// col-row size of chiBuffer 
		coordBufCoord,		// top left src cell in chiBuffer 
		&writeRect);	// dest. screen textBuffer rectangle 
}

void
drawf_line(const v2i pos, const char *format, ...)
{
	char buffer[80];
	va_list argp;
	va_start(argp, format);
	vsprintf_s(buffer, 80, format, argp);
	draw_line(pos, buffer);
	va_end(argp);
}

void
draw_char(const v2i pos, const char c)
{
	CHAR_INFO chiBuffer = {0};
	chiBuffer.Attributes = 0x07;
	chiBuffer.Char.AsciiChar = c;

	SMALL_RECT writeRect = { .Left = (short)pos.x, .Top = (short)pos.y, .Right = (short)pos.x + 1, .Bottom = (short)pos.y + 1 };
	WriteConsoleOutput(
		_hBackBuffer,
		&chiBuffer,
		(COORD) { 1, 1 },
		(COORD) { 0 },
		&writeRect);
}

int
file_open_for_write(const char* filename, file_t** file)
{
	FILE* fs;
	if (fopen_s(&fs, filename, "wb"))
		return 0;

	win32_file_t* win32_file = (win32_file_t*)transient_alloc(sizeof(win32_file_t));
	win32_file->fileInfo.size = 0;
	win32_file->fs = fs;
	win32_file->buffer = ((char*)win32_file) + sizeof(win32_file_t);

	(*file) = (file_t*)win32_file;
	return 1;
}

int
file_open_for_read(const char* filename, file_t** file)
{
	FILE* fs;
	if (fopen_s(&fs, filename, "rb"))
		return 0;

	struct stat st;
	stat(filename, &st);
	long size = st.st_size;

	win32_file_t* win32_file = (win32_file_t*)transient_alloc(sizeof(win32_file_t) + READBUFFERSIZE);
	(*win32_file) = (win32_file_t ){ 0 };
	win32_file->fileInfo.size = size;
	win32_file->fs = fs;
	win32_file->buffer = ((char*)win32_file) + sizeof(win32_file_t);
	
	(*file) = (file_t* )win32_file;
	return 1;
}

void
file_write(const file_t* file, const char* line)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	int bytes = fputs(line, win32_file->fs);
	win32_file->fileInfo.size += bytes;
}

char* 
file_read(const file_t* file)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	return fgets(win32_file->buffer, READBUFFERSIZE, win32_file->fs);
}

long
file_seek(const file_t* file, long offset)
{
	win32_file_t* win32_file = (win32_file_t*)file;
	if (offset < 0)
	{
		return fseek(win32_file->fs, offset, SEEK_END);
	}
	
	return fseek(win32_file->fs, offset, SEEK_SET);
}

void 
file_free(const file_t* file) 
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

	LPVOID BaseAddress = (LPVOID)0x2000000;
	byte* base = (byte* )VirtualAlloc(BaseAddress, GAME_HEAP_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	memory_init(base, GAME_HEAP_SIZE);
	init_game();

	_alive = true;
	while (_alive)
	{
		transient_reset();
		beginRender();
		update_and_render();
		renderComplete();

		game_input_t input = readInput();
		process_input(input);
	}

	win32_close();
	return 0;
}