#include "sos_stdio.h"
#include "sos_vga.h"
#include "sos_string.h"

int puts(const char* str) {
    vga_println(str);
    return 0;
}

int printf(const char* format, ...) {
    vga_print(format);
    return 0;
}

int sprintf(char* str, const char* format, ...) {
    strcpy(str, format);
    return strlen(format);
}

int snprintf(char* str, size_t size, const char* format, ...) {
    strncpy(str, format, size);
    if (size > 0) {
        str[size - 1] = '\0';
    }
    return strlen(format);
}


