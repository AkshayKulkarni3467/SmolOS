#include "sos_memory.h"

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (unsigned char)value;
    }
    return ptr;
}

void* memcpy(void* destination, const void* source, size_t num) {
    unsigned char* dst = (unsigned char*)destination;
    const unsigned char* src = (const unsigned char*)source;
    for (size_t i = 0; i < num; i++) {
        dst[i] = src[i];
    }
    return destination;
}

void* memmove(void* destination, const void* source, size_t num) {
    unsigned char* dst = (unsigned char*)destination;
    const unsigned char* src = (const unsigned char*)source;
    
    if (dst < src) {
        for (size_t i = 0; i < num; i++) {
            dst[i] = src[i];
        }
    } else {
        for (size_t i = num; i > 0; i--) {
            dst[i - 1] = src[i - 1];
        }
    }
    return destination;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    
    for (size_t i = 0; i < num; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}