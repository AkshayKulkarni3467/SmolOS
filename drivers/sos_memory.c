#include "sos_memory.h"

#ifdef SMOLOS_MEMORY_TEST
#include "sos_stdio.h"
#include <assert.h>
#include "sos_string.h"
#endif

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

#ifdef SMOLOS_MEMORY_TEST

void test_memset() {
    printf("Testing memset...\n");
    
    unsigned char buf[100];
    
    memset(buf, 0, 100);
    for (int i = 0; i < 100; i++) {
        assert(buf[i] == 0);
    }
    
    memset(buf, 0xAA, 50);
    for (int i = 0; i < 50; i++) {
        assert(buf[i] == 0xAA);
    }
    
    printf("[T] memset works correctly\n");
}

void test_memcpy() {
    printf("Testing memcpy...\n");
    
    char src[] = "Hello World";
    char dst[20];
    
    memcpy(dst, src, 12);
    assert(memcmp(dst, src, 12) == 0);
    
    unsigned char data1[] = {1, 2, 3, 4, 5};
    unsigned char data2[5];
    memcpy(data2, data1, 5);
    assert(memcmp(data1, data2, 5) == 0);
    
    printf("[T] memcpy works correctly\n");
}

void test_memmove() {
    printf("Testing memmove...\n");
    
    char buf[20] = "Hello World";
    
    memmove(buf + 2, buf, 5);
    assert(buf[2] == 'H');
    assert(buf[3] == 'e');
    
    strcpy(buf, "Hello World");
    memmove(buf, buf + 2, 5);
    assert(buf[0] == 'l');
    assert(buf[1] == 'l');
    
    printf("[T] memmove works correctly\n");
}

void test_memcmp() {
    printf("Testing memcmp...\n");
    
    assert(memcmp("abc", "abc", 3) == 0);
    assert(memcmp("abc", "abd", 3) < 0);
    assert(memcmp("abd", "abc", 3) > 0);
    
    unsigned char a[] = {1, 2, 3};
    unsigned char b[] = {1, 2, 3};
    unsigned char c[] = {1, 2, 4};
    
    assert(memcmp(a, b, 3) == 0);
    assert(memcmp(a, c, 3) < 0);
    
    printf("[T] memcmp works correctly\n");
}

void test_memchr() {
    printf("Testing memchr...\n");
    
    char str[] = "Hello World";
    
    char* result = (char*)memchr(str, 'W', 11);
    assert(result == str + 6);
    
    result = (char*)memchr(str, 'o', 11);
    assert(result == str + 4);
    
    result = (char*)memchr(str, 'x', 11);
    assert(result == 0);
    
    printf("[T] memchr works correctly\n");
}

void test_memswap() {
    printf("Testing memswap...\n");
    
    unsigned char a[] = {1, 2, 3, 4, 5};
    unsigned char b[] = {10, 20, 30, 40, 50};
    unsigned char a_copy[5];
    unsigned char b_copy[5];
    
    memcpy(a_copy, a, 5);
    memcpy(b_copy, b, 5);
    
    memswap(a, b, 5);
    
    assert(memcmp(a, b_copy, 5) == 0);
    assert(memcmp(b, a_copy, 5) == 0);
    
    printf("[T] memswap works correctly\n");
}

void test_memrev() {
    printf("Testing memrev...\n");
    
    unsigned char data[] = {1, 2, 3, 4, 5};
    memrev(data, 5);
    
    unsigned char expected[] = {5, 4, 3, 2, 1};
    assert(memcmp(data, expected, 5) == 0);
    
    char str[] = "Hello";
    memrev(str, 5);
    assert(memcmp(str, "olleH", 5) == 0);
    
    printf("[T] memrev works correctly\n");
}

void test_memcount() {
    printf("Testing memcount...\n");
    
    unsigned char data[] = {1, 2, 1, 3, 1, 4, 1};
    assert(memcount(data, 1, 7) == 4);
    assert(memcount(data, 2, 7) == 1);
    assert(memcount(data, 5, 7) == 0);
    
    char str[] = "Hello World";
    assert(memcount(str, 'l', 11) == 3);
    assert(memcount(str, 'o', 11) == 2);
    
    printf("[T] memcount works correctly\n");
}

int main(void) {
    printf("=== SmolOS Memory Library Test Suite ===\n\n");
    
    test_memset();
    test_memcpy();
    test_memmove();
    test_memcmp();
    test_memchr();
    test_memswap();
    test_memrev();
    test_memcount();
    
    printf("\n=== All memory tests passed! ===\n");
    return 0;
}

#endif