#include <windows.h>
#include <stdio.h>
#include <tchar.h>

static HANDLE hStdin, hStdout, hActiveBuffer, hBackBuffer;
static int fdwSaveOldMode;
static bool alive;

short x = 9, y = 9;

void debug(wchar_t* outputString)
{
	OutputDebugString(outputString);
}

void debug(char* outputString)
{
	OutputDebugStringA(outputString);
}

void debugf(const wchar_t* format, ...)
{
	va_list argp; 
	va_start(argp, format);
	wchar_t buffer[512];
	vswprintf_s(buffer, 512, format, argp);
	OutputDebugString(buffer);
	va_end(argp);
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

VOID KeyEventProc(KEY_EVENT_RECORD ker)
{
	debugf("Key event: ");

	if (ker.bKeyDown)
	{
		debugf("key pressed '%c'\n", ker.uChar.AsciiChar);
	}
	else 
	{
		switch (ker.wVirtualKeyCode)
		{
		case 0x51:
			alive = false;
			break;

		case VK_DOWN:
			y++;
			break;

		case VK_UP:
			y--;
			break;

		case VK_LEFT:
			x--;
			break;

		case VK_RIGHT:
			x++;
			break;

		default:
			debugf("key released '%c'\n", ker.uChar.AsciiChar);
		}
	}
}

int processInput()
{
	DWORD cNumRead, i;
	INPUT_RECORD irInBuf[128];

	if (!ReadConsoleInput(
		hStdin,      // input buffer handle 
		irInBuf,     // buffer to read into 
		128,         // size of read buffer 
		&cNumRead))  // number of records read 
	{
		return 1;
	}

	for (i = 0; i < cNumRead; i++)
	{
		switch (irInBuf[i].EventType)
		{
		case KEY_EVENT: // keyboard input 
			KeyEventProc(irInBuf[i].Event.KeyEvent);
			break;

		default:
			break;
		}
	}

	return 0;
}

int beginRender()
{
	DWORD write;
	FillConsoleOutputCharacter(hBackBuffer, _T(' '), 80 * 25, {}, &write);
	return 0;
}

int renderComplete()
{
	HANDLE hPreviousBuffer = hActiveBuffer;
	hActiveBuffer = hBackBuffer;
	
	SetConsoleActiveScreenBuffer(hActiveBuffer);
	hBackBuffer = hPreviousBuffer;

	return 0;
}

int win32_close()
{
	SetConsoleMode(hStdin, fdwSaveOldMode);	
	return SetConsoleActiveScreenBuffer(hStdout);
}

int drawToBuffer(char *text)
{
	COORD coordBufCoord = {}, coordBufSize = { 80, 2 };
	SMALL_RECT srctWriteRect;

	CHAR_INFO chiBuffer[160]; 
	memset(chiBuffer, 0, sizeof(chiBuffer));

	for (int i = 0; i < 160 && *text; i++, text++)
	{
		chiBuffer[i].Char.AsciiChar = *text;
		chiBuffer[i].Attributes = 0xf7;
	}

	srctWriteRect.Top = 0;    // top lt: row 10, col 0 
	srctWriteRect.Left = 0;
	srctWriteRect.Bottom = 2; // bot. rt: row 11, col 79 
	srctWriteRect.Right = 80;

	// Copy from the temporary buffer to the new screen buffer. 
	BOOL fSuccess = WriteConsoleOutput(
		hBackBuffer,		// screen buffer to write to 
		chiBuffer,			// buffer to copy from 
		coordBufSize,		// col-row size of chiBuffer 
		coordBufCoord,		// top left src cell in chiBuffer 
		&srctWriteRect);	// dest. screen buffer rectangle 

	if (fSuccess)
	{
		renderComplete();
		return 0;
	}

	printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
	return 1;
}

int drawCharAt(short x, short y, char c)
{
	CHAR_INFO chiBuffer = {};
	chiBuffer.Attributes = 0x07;
	chiBuffer.Char.AsciiChar = c;

	SMALL_RECT writeRect;
	writeRect.Left = x;
	writeRect.Top = y;
	writeRect.Bottom = y + 1;
	writeRect.Right = x + 1;

	WriteConsoleOutput(
		hBackBuffer,
		&chiBuffer,
		{ 1, 1 },
		{},
		&writeRect);

	return 0;
}

int _tmain()
{
	if (win32_init())
		return 1;

	alive = true;
	while (alive)
	{
		beginRender();

		bool bumpIntoWall = false;
		if (x < 0)
		{
			x = 0;
			bumpIntoWall = true;
		}
		if (y < 0)
		{
			y = 0;
			bumpIntoWall = true;
		}
		if (y > 19)
		{
			y = 19;
			bumpIntoWall = true;
		}
		if (x > 19)
		{
			x = 19;
			bumpIntoWall = true;
		}
		if (bumpIntoWall)
		{
			drawToBuffer("You bumped into a wall");
		}

		drawCharAt(x + 30, y + 2, '@');
		renderComplete();

		processInput();
	}

	win32_close();
	return 0;
}