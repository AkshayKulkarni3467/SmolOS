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

/*TODO : Implement file management later?*/
FILE* fopen(char* filename, char* mode) { return 0; }
int fclose(FILE* stream) { return 0; }
size_t fread(void* ptr, size_t size, size_t count, FILE* stream) { return 0; }
size_t fwrite(void* ptr, size_t size, size_t count, FILE* stream) { return 0; }
int fseek(FILE* stream, long offset, int whence) { return 0; }
long ftell(FILE* stream) { return 0; }

