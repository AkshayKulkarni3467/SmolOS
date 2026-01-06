#include "sos_vga.h"

void kernel_main(void) {

    vga_init();
    vga_clear();
    vga_println("Booting SmolOS...");


    while (1) {}
}