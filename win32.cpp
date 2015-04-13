#include <windows.h>
#include <stdio.h>
#include <tchar.h>

static HANDLE hStdout, hActiveBuffer, hBackBuffer;

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
		FILE_SHARE_READ | FILE_SHARE_WRITE,			// shared 
		NULL,										// default security attributes 
		CONSOLE_TEXTMODE_BUFFER,					// must be TEXTMODE 
		NULL										// reserved; must be NULL 
		);

	if (hStdout == INVALID_HANDLE_VALUE || hActiveBuffer == INVALID_HANDLE_VALUE || hBackBuffer == INVALID_HANDLE_VALUE)
	{
		printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
		return 1;
	}

	// Make the new screen buffer the active screen buffer. 

	if (SetConsoleActiveScreenBuffer(hActiveBuffer))
	{
		return 0;
	}

	printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
	return 1;
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

	srctWriteRect.Top = 10;    // top lt: row 10, col 0 
	srctWriteRect.Left = 0;
	srctWriteRect.Bottom = 11; // bot. rt: row 11, col 79 
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

		Sleep(1000);
		return 0;
	}

	printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (win32_init())
		return 1;

	drawToBuffer("Hello world");

	win32_close();
	return 0;
}