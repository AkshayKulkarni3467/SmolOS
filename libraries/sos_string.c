#include "sos_string.h"


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

char* strchr(char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return str;
        }
        str++;
    }
    return (*str == (char)c) ? str : 0;
}

char* strrchr(char* str, int c) {
    char* last = 0;
    while (*str) {
        if (*str == (char)c) {
            last = str;
        }
        str++;
    }
    if (*str == (char)c) {
        return str;
    }
    return last;
}

char* strstr(char* haystack, char* needle) {
    if (!*needle) return haystack;
    
    while (*haystack) {
        char* h = haystack;
        char* n = needle;
        
        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }
        
        if (!*n) return haystack;
        haystack++;
    }
    
    return 0;
}

void strrev(char* str) {
    if (!str) return;
    
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - 1 - i];
        str[len - 1 - i] = temp;
    }
}

void strtoupper(char* str) {
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - 'a' + 'A';
        }
        str++;
    }
}

void strtolower(char* str) {
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str - 'A' + 'a';
        }
        str++;
    }
}

int strstartswith(char* str, char* prefix) {
    while (*prefix) {
        if (*str != *prefix) {
            return 0;
        }
        str++;
        prefix++;
    }
    return 1;
}

int strendswith(char* str, char* suffix) {
    int str_len = strlen(str);
    int suffix_len = strlen(suffix);
    
    if (suffix_len > str_len) {
        return 0;
    }
    
    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

void strtrim(char* str) {
    if (!str || !*str) return;
    
    char* start = str;
    while (*start == ' ' || *start == '\t' || *start == '\n') {
        start++;
    }
    
    char* end = str + strlen(str) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n')) {
        end--;
    }
    

    size_t len = end - start + 1;
    for (size_t i = 0; i < len; i++) {
        str[i] = start[i];
    }
    str[len] = '\0';
}

