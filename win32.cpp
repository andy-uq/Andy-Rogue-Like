#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "arl.h"

HANDLE hStdin, hStdout, hActiveBuffer, hBackBuffer;
int fdwSaveOldMode;
bool alive;

void debug(wchar_t* outputString)
{
	OutputDebugStringW(outputString);
}

void debugf(const wchar_t* format, ...)
{
	va_list argp; 
	va_start(argp, format);
	wchar_t buffer[512];
	vswprintf_s(buffer, 512, format, argp);
	OutputDebugStringW(buffer);
	va_end(argp);
}

void debug(char* outputString)
{
	OutputDebugStringA(outputString);
}

void debugf(const char* format, ...)
{
	char buffer[512];
	va_list argp;
	va_start(argp, format);
	vsprintf_s(buffer, 512, format, argp);
	OutputDebugStringA(buffer);
	va_end(argp);
}

internal
int win32_init()
{
	// Get a handle to the STDOUT screen buffer to copy from and 
	// create a new screen buffer to copy to. 

	hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	hBackBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,				// read/write access
		FILE_SHARE_READ | FILE_SHARE_WRITE,			// shared 
		NULL,										// default security attributes 
		CONSOLE_TEXTMODE_BUFFER,					// must be TEXTMODE 
		NULL										// reserved; must be NULL 
		);

	hActiveBuffer = CreateConsoleScreenBuffer(
		GENERIC_READ | GENERIC_WRITE,				// read/write access
		FILE_SHARE_READ | FILE_SHARE_WRITE, 		// shared 
		NULL, 										// default security attributes 
		CONSOLE_TEXTMODE_BUFFER, 					// must be TEXTMODE 
		NULL 										// reserved; must be NULL 
		);

	if (hStdout == INVALID_HANDLE_VALUE || hActiveBuffer == INVALID_HANDLE_VALUE || hBackBuffer == INVALID_HANDLE_VALUE)
	{
		debugf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
		return 1;
	}

	// Make the new screen buffer the active screen buffer. 

	if (!SetConsoleActiveScreenBuffer(hActiveBuffer))
	{
		debugf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
		return 1;
	}

	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin == INVALID_HANDLE_VALUE)
	{
		debugf("GetStdHandle failed - (%d)\n", GetLastError());
		return 1;
	}

	int fdwMode = 0;
	if (!SetConsoleMode(hStdin, fdwMode))
	{
		debugf("SetConsoleMode failed - (%d)\n", GetLastError());
		return 1;
	}

	CONSOLE_CURSOR_INFO cursorInfo = {};
	cursorInfo.bVisible = false;
	cursorInfo.dwSize = 100;

	SetConsoleCursorInfo(hActiveBuffer, &cursorInfo);
	SetConsoleCursorInfo(hBackBuffer, &cursorInfo);
	return 0;
}

internal
game_input* KeyEventProc(KEY_EVENT_RECORD ker, game_input* input)
{
	debugf("key %s '%c'\n", (ker.bKeyDown ? "pressed" : "released"), ker.uChar.AsciiChar);
	switch (ker.wVirtualKeyCode)
	{
		case 'Q':
			if (!ker.bKeyDown)
			{
				alive = false;
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
		hStdin,      // input buffer handle 
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
	FillConsoleOutputCharacter(hBackBuffer, _T(' '), 80 * 25, {}, &write);
	return 0;
}

internal
int renderComplete()
{
	HANDLE hPreviousBuffer = hActiveBuffer;
	hActiveBuffer = hBackBuffer;
	
	SetConsoleActiveScreenBuffer(hActiveBuffer);
	hBackBuffer = hPreviousBuffer;

	return 0;
}

internal
int win32_close()
{
	SetConsoleMode(hStdin, fdwSaveOldMode);	
	return SetConsoleActiveScreenBuffer(hStdout);
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
		hBackBuffer,		// screen textBuffer to write to 
		textBuffer,			// textBuffer to copy from 
		coordBufSize,		// col-row size of chiBuffer 
		coordBufCoord,		// top left src cell in chiBuffer 
		&srctWriteRect);	// dest. screen textBuffer rectangle 
}

void
drawCharAt(const screen_coord pos, char c)
{
	CHAR_INFO chiBuffer = {};
	chiBuffer.Attributes = 0x07;
	chiBuffer.Char.AsciiChar = c;

	SMALL_RECT writeRect = { pos.x, pos.y, pos.x + 1, pos.y + 1 };
	WriteConsoleOutput(
		hBackBuffer,
		&chiBuffer,
		{ 1, 1 },
		{},
		&writeRect);
}

int 
_tmain()
{
	if (win32_init())
		return 1;

	init_game();

	alive = true;
	while (alive)
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