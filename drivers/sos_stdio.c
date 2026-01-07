#include "sos_stdio.h"
#include "sos_vga.h"

int printf(const char* format, ...) {
    vga_print(format);
    return 0;
}

