#pragma once

/* rendering */
void drawToBuffer(const char* text);
void drawCharAt(const v2i pos, const char);
void drawLineAt(const v2i pos, const char* text);
void drawfLineAt(const v2i pos, const char* format, ...);

/* debug */
void debug(char* outputString);
void debugf(const char* format, ...);

/* file */
int openFileForWrite(const char* filename, file_t** file);
void writeLine(const file_t* file, const char* line);

int openFileForRead(const char* filename, file_t** file);
char* readLine(const file_t* file);

long seek(const file_t* file, long offset);
void freeFile(const file_t* file);
