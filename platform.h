#pragma once

/* rendering */
void draw_to_buffer(const char* text);
void draw_char(const v2i pos, const char);
void draw_line(const v2i pos, const char* text);
void drawf_line(const v2i pos, const char* format, ...);

/* debug */
void debug(char* outputString);
void debugf(const char* format, ...);

/* file */
int file_open_for_write(const char* filename, file_t** file);
void file_write(const file_t* file, const char* line);

int file_open_for_read(const char* filename, file_t** file);
char* file_read(const file_t* file);

long file_seek(const file_t* file, long offset);
void file_free(const file_t* file);
