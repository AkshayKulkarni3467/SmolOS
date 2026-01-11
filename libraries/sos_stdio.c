#include "sos_stdio.h"
#include "sos_vga.h"
#include "sos_string.h"

int puts(char* str) {
    vga_println(str);
    return 0;
}

int printf(char* format, ...) {
    vga_print(format);
    return 0;
}

int sprintf(char* str, char* format, ...) {
    strcpy(str, format);
    return strlen(format);
}

int snprintf(char* str, size_t size, char* format, ...) {
    strncpy(str, format, size);
    if (size > 0) {
        str[size - 1] = '\0';
    }
    return strlen(format);
}


