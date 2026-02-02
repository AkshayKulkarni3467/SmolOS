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

void* memchr(const void* ptr, int value, size_t num) {
    const unsigned char* p = (const unsigned char*)ptr;
    for (size_t i = 0; i < num; i++) {
        if (p[i] == (unsigned char)value) {
            return (void*)(p + i);
        }
    }
    return 0;
}

void memswap(void* ptr1, void* ptr2, size_t num) {
    unsigned char* p1 = (unsigned char*)ptr1;
    unsigned char* p2 = (unsigned char*)ptr2;
    
    for (size_t i = 0; i < num; i++) {
        unsigned char temp = p1[i];
        p1[i] = p2[i];
        p2[i] = temp;
    }
}

void memrev(void* ptr, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < num / 2; i++) {
        unsigned char temp = p[i];
        p[i] = p[num - 1 - i];
        p[num - 1 - i] = temp;
    }
}

int memcount(const void* ptr, int value, size_t num) {
    const unsigned char* p = (const unsigned char*)ptr;
    int count = 0;
    
    for (size_t i = 0; i < num; i++) {
        if (p[i] == (unsigned char)value) {
            count++;
        }
    }
    
    return count;
}
