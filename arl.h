#pragma once

#define internal static

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
