#ifndef INCLUDE_SMOLOS_MEMORY_H
#define INCLUDE_SMOLOS_MEMORY_H

#include "sos_stddef.h"

void* memset(void* ptr, int value, size_t num);
void* memcpy(void* destination, const void* source, size_t num);
void* memmove(void* destination, const void* source, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

#endif //INCLUDE_SMOLOS_MEMORY_H

