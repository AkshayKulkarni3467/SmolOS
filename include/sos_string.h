#ifndef INCLUDE_SMOLOS_STRING_H
#define INCLUDE_SMOLOS_STRING_H

#include "sos_stddef.h"

size_t strlen(char* str);
char* strcpy(char* destination, char* source);
char* strncpy(char* destination, char* source, size_t num);
int strcmp(char* str1, char* str2);
int strncmp(char* str1, char* str2, size_t num);
char* strcat(char* destination, char* source);

char* strchr(char* str, int c);
char* strrchr(char* str, int c);
char* strstr(char* haystack, char* needle);
void strrev(char* str);
void strtoupper(char* str);
void strtolower(char* str);
int strstartswith(char* str, char* prefix);
int strendswith(char* str, char* suffix);
void strtrim(char* str);

#endif //INCLUDE_SMOLOS_STRING_H