#ifndef INCLUDE_SMOLOS_STDIO_H
#define INCLUDE_SMOLOS_STDIO_H

#include "sos_stddef.h"

int puts(char* str);
int printf(char* format, ...);
int sprintf(char* str, char* format, ...);
int snprintf(char* str, size_t size, char* format, ...);

typedef struct FILE FILE;
FILE* fopen(char* filename, char* mode);
int fclose(FILE* stream);
size_t fread(void* ptr, size_t size, size_t count, FILE* stream);
size_t fwrite(void* ptr, size_t size, size_t count, FILE* stream);
int fseek(FILE* stream, long offset, int whence);
long ftell(FILE* stream);

#endif //INCLUDE_SMOLOS_STDIO_H
