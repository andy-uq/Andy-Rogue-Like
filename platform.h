#pragma once

/* memory */
void* allocate(size_t size);
char* alloc_s(const char* source);
void* allocateTransient(size_t size);

memoryArena_t* allocateArena(size_t size);
void freeArena(memoryArena_t* arena);
void* arenaAlloc(memoryArena_t** arena, size_t size);

/* rendering */
void drawToBuffer(const char* text);
void drawCharAt(const v2i pos, const char);
void drawLineAt(const v2i pos, const char* text);
void drawfLineAt(const v2i pos, const char* format, ...);

/* debug */
void debug(wchar_t* outputString);
void debug(char* outputString);
void debugf(const wchar_t* format, ...);
void debugf(const char* format, ...);

/* file */
int openFileForWrite(const char* filename, file_t** file);
void writeLine(const file_t* file, const char* line);

int openFileForRead(const char* filename, file_t** file);
char* readLine(const file_t* file);

long seek(const file_t* file, long offset);
void freeFile(const file_t* file);
