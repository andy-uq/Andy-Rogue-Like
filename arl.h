#pragma once

#define internal static

#define NULL 0

struct v2i {
	int x, y;
};

struct game_input {
	short xOffset, yOffset;
	bool quit;
};

void init_game();
void updateAndRender();
void processInput(const game_input input);

void drawToBuffer(const char* text);
void drawCharAt(const v2i pos, const char);

void debug(wchar_t* outputString);
void debug(char* outputString);
void debugf(const wchar_t* format, ...);
void debugf(const char* format, ...);

struct file_t
{
	int size;
};

int openFileForWrite(const char* filename, file_t** file);
void writeLine(const file_t* file, const char* line);

int openFileForRead(const char* filename, file_t** file);
char* readLine(const file_t* file);

void freeFile(const file_t* file);

void* allocate(size_t size);