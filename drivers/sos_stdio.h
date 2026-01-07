#ifndef INCLUDE_SMOLOS_STDIO_H
#define INCLUDE_SMOLOS_STDIO_H

#include <stddef.h>

int puts(const char* str);
int printf(const char* format, ...);
int sprintf(char* str, const char* format, ...);
int snprintf(char* str, size_t size, const char* format, ...);

#endif //INCLUDE_SMOLOS_STDIO_H
