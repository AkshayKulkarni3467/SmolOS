#include "sos_vga.h"

#ifdef SMOLOS_KERNEL_TEST

#include "sos_stdio.h"

#endif

void kernel_main(void) {

    vga_init();
    vga_clear();
    vga_println("Booting SmolOS...");


    while (1) {}
}

#ifdef SMOLOS_KERNEL_TEST

int main(void){
    printf("Hello from kernel.c\n");
    return 0;
}

#endif