#include "sos_string.h"

#ifdef SMOLOS_STRING_TEST
#include "sos_stdio.h"
#include "sos_string.h"
#include <assert.h>
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

#ifdef SMOLOS_STRING_TEST

void test_strlen() {
    printf("Testing strlen...\n");
    
    assert(strlen("") == 0);
    assert(strlen("Hello") == 5);
    assert(strlen("SmolOS") == 6);
    assert(strlen("A") == 1);
    
    printf("[T] strlen works correctly\n");
}

void test_strcpy() {
    printf("Testing strcpy...\n");
    
    char buf[100];
    
    strcpy(buf, "Hello");
    assert(strcmp(buf, "Hello") == 0);
    
    strcpy(buf, "");
    assert(strcmp(buf, "") == 0);
    
    strcpy(buf, "SmolOS is great!");
    assert(strcmp(buf, "SmolOS is great!") == 0);
    
    printf("[T] strcpy works correctly\n");
}

void test_strncpy() {
    printf("Testing strncpy...\n");
    
    char buf[100];
    
    strncpy(buf, "Hello", 3);
    buf[3] = '\0';
    assert(strcmp(buf, "Hel") == 0);
    
    strncpy(buf, "World", 10);
    assert(strcmp(buf, "World") == 0);
    
    printf("[T] strncpy works correctly\n");
}

void test_strcmp() {
    printf("Testing strcmp...\n");
    
    assert(strcmp("abc", "abc") == 0);
    assert(strcmp("abc", "abd") < 0);
    assert(strcmp("abd", "abc") > 0);
    assert(strcmp("", "") == 0);
    assert(strcmp("a", "") > 0);
    
    printf("[T] strcmp works correctly\n");
}

void test_strncmp() {
    printf("Testing strncmp...\n");
    
    assert(strncmp("Hello", "Help", 3) == 0);
    assert(strncmp("Hello", "Help", 4) < 0);
    assert(strncmp("abc", "xyz", 1) < 0);
    
    printf("[T] strncmp works correctly\n");
}

void test_strcat() {
    printf("Testing strcat...\n");
    
    char buf[100] = "Hello";
    strcat(buf, " World");
    assert(strcmp(buf, "Hello World") == 0);
    
    strcpy(buf, "");
    strcat(buf, "Test");
    assert(strcmp(buf, "Test") == 0);
    
    printf("[T] strcat works correctly\n");
}

void test_strchr() {
    printf("Testing strchr...\n");
    
    char* str = "Hello World";
    assert(strchr(str, 'W') == str + 6);
    assert(strchr(str, 'o') == str + 4);
    assert(strchr(str, 'x') == 0);
    assert(strchr(str, '\0') == str + 11);
    
    printf("[T] strchr works correctly\n");
}

void test_strrchr() {
    printf("Testing strrchr...\n");
    
    char* str = "Hello World";
    assert(strrchr(str, 'o') == str + 7);  
    assert(strrchr(str, 'H') == str);
    assert(strrchr(str, 'x') == 0);
    
    printf("[T] strrchr works correctly\n");
}

void test_strstr() {
    printf("Testing strstr...\n");
    
    char* str = "Hello World";
    assert(strstr(str, "World") == str + 6);
    assert(strstr(str, "Hello") == str);
    assert(strstr(str, "xyz") == 0);
    assert(strstr(str, "") == str);
    
    printf("[T] strstr works correctly\n");
}

void test_strrev() {
    printf("Testing strrev...\n");
    
    char buf[100];
    
    strcpy(buf, "Hello");
    strrev(buf);
    assert(strcmp(buf, "olleH") == 0);
    
    strcpy(buf, "a");
    strrev(buf);
    assert(strcmp(buf, "a") == 0);
    
    strcpy(buf, "");
    strrev(buf);
    assert(strcmp(buf, "") == 0);
    
    printf("[T] strrev works correctly\n");
}

void test_strtoupper() {
    printf("Testing strtoupper...\n");
    
    char buf[100];
    
    strcpy(buf, "hello");
    strtoupper(buf);
    assert(strcmp(buf, "HELLO") == 0);
    
    strcpy(buf, "HeLLo123");
    strtoupper(buf);
    assert(strcmp(buf, "HELLO123") == 0);
    
    printf("[T] strtoupper works correctly\n");
}

void test_strtolower() {
    printf("Testing strtolower...\n");
    
    char buf[100];
    
    strcpy(buf, "HELLO");
    strtolower(buf);
    assert(strcmp(buf, "hello") == 0);
    
    strcpy(buf, "HeLLo123");
    strtolower(buf);
    assert(strcmp(buf, "hello123") == 0);
    
    printf("[T] strtolower works correctly\n");
}

void test_strstartswith() {
    printf("Testing strstartswith...\n");
    
    assert(strstartswith("Hello World", "Hello") == 1);
    assert(strstartswith("Hello World", "World") == 0);
    assert(strstartswith("Hello", "Hello") == 1);
    assert(strstartswith("Hi", "Hello") == 0);
    
    printf("[T] strstartswith works correctly\n");
}

void test_strendswith() {
    printf("Testing strendswith...\n");
    
    assert(strendswith("Hello World", "World") == 1);
    assert(strendswith("Hello World", "Hello") == 0);
    assert(strendswith("World", "World") == 1);
    assert(strendswith("Hi", "Hello") == 0);
    
    printf("[T] strendswith works correctly\n");
}

void test_strtrim() {
    printf("Testing strtrim...\n");
    
    char buf[100];
    
    strcpy(buf, "  Hello  ");
    strtrim(buf);
    assert(strcmp(buf, "Hello") == 0);
    
    strcpy(buf, "NoSpaces");
    strtrim(buf);
    assert(strcmp(buf, "NoSpaces") == 0);
    
    strcpy(buf, "\t\n Hello \t\n");
    strtrim(buf);
    assert(strcmp(buf, "Hello") == 0);
    
    printf("[T] strtrim works correctly\n");
}

int main(void) {
    printf("=== SmolOS String Library Test Suite ===\n\n");
    
    test_strlen();
    test_strcpy();
    test_strncpy();
    test_strcmp();
    printf("\n=== All string tests passed! ===\n");
    return 0;
}
#endif