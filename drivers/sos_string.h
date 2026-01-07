#ifndef INCLUDE_SMOLOS_STRING_H
#define INCLUDE_SMOLOS_STRING_H

#include "sos_stddef.h"

size_t strlen(const char* str);
char* strcpy(char* destination, const char* source);
char* strncpy(char* destination, const char* source, size_t num);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t num);
char* strcat(char* destination, const char* source);

#endif //INCLUDE_SMOLOS_STRING_H