#ifndef INCLUDE_SMOLOS_STDIO_H
#define INCLUDE_SMOLOS_STDIO_H

#include "sos_stddef.h"

int puts(char* str);
int printf(char* format, ...);
int sprintf(char* str, char* format, ...);
int snprintf(char* str, size_t size, char* format, ...);


#endif //INCLUDE_SMOLOS_STDIO_H
