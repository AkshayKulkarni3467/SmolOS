#ifndef INCLUDE_SMOLOS_STDIO_H
#define INCLUDE_SMOLOS_STDIO_H

#include "sos_stddef.h"

int puts(const char* str);
int printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);

typedef struct FILE FILE;
FILE* fopen(const char* filename, const char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
size_t fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);

#endif //INCLUDE_SMOLOS_STDIO_H
