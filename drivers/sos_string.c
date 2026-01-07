#include "sos_string.h"

#ifdef SMOLOS_STRING_TEST

#include "sos_stdio.h"

#endif

size_t strlen(char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

char* strcpy(char* destination, char* source) {
    char* dst = destination;
    while (*source) {
        *dst++ = *source++;
    }
    *dst = '\0';
    return destination;
}

char* strncpy(char* destination, char* source, size_t num) {
    char* dst = destination;
    size_t i;
    
    for (i = 0; i < num && source[i]; i++) {
        dst[i] = source[i];
    }
    
    for (; i < num; i++) {
        dst[i] = '\0';
    }
    
    return destination;
}

int strcmp(char* str1, char* str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

int strncmp(char* str1, char* str2, size_t num) {
    for (size_t i = 0; i < num; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
        if (str1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

char* strcat(char* destination, char* source) {
    char* dst = destination;
    while (*dst) dst++;
    while (*source) *dst++ = *source++;
    *dst = '\0';
    return destination;
}

#ifdef SMOLOS_STRING_TEST

int main(void){
    printf("Hello from string.c\n");
    return 0;
}

#endif