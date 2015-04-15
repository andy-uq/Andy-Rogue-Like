#pragma once

#define internal static

struct screen_coord {
	short x, y;
};

struct game_input {
	short xOffset, yOffset;
	bool quit;
};

void updateAndRender();
void processInput(const game_input input);

void drawToBuffer(const char* text);
void drawCharAt(const screen_coord pos, const char);

void debug(wchar_t* outputString);
void debug(char* outputString);
void debugf(const wchar_t* format, ...);
void debugf(const char* format, ...);
