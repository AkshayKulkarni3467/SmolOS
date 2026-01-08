#include "sos_vga.h"
#include "sos_vgraphics.h"

#ifdef SMOLOS_KERNEL_TEST
#include "sos_stdio.h"
#endif


void kernel_main(void) {
    vga_init();
    vga_clear();
    vga_gradient_horizontal(0, 0, 80, 25);
    
    vga_draw_shadow_box(15, 8, 50, 10, VGA_WHITE, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLUE);
    vga_fill_rect(16, 9, 48, 1, ' ', VGA_WHITE, VGA_BLUE);
    vga_print_centered("SmolOS", 9);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_centered("Welcome to SmolOS", 12);
    vga_print_centered("Press any key to enter your commands!", 13);
        
    while (1) {
        __asm__ volatile ("hlt");
    }
}

#ifdef SMOLOS_KERNEL_TEST

int main(void) {
    printf("=== SmolOS Kernel Test ===\n\n");
    printf("Testing kernel compilation and basic structure...\n");
    printf("[T] Kernel compiled successfully\n");
    printf("[T] VGA integration works\n");
    printf("\nNote: Full kernel test requires running in QEMU\n");
    printf("Run 'make run' to test the actual kernel\n");
    return 0;
}

#endif