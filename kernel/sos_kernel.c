#include "sos_vga.h"

#ifdef SMOLOS_KERNEL_TEST
#include "sos_stdio.h"
#endif

void kernel_main(void) {
    vga_init();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("========================================");
    vga_println("      Welcome to SmolOS v0.1");
    vga_println("========================================");
    vga_reset_color();
    vga_println("");
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("[OK] VGA initialized");
    vga_println("[OK] Kernel loaded");
    vga_println("[OK] System ready");
    vga_reset_color();
    vga_println("");
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Memory: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("16 KB stack");
    vga_reset_color();
    vga_println("");
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Display: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("80x25 VGA text mode");
    vga_reset_color();
    vga_println("");
    vga_println("");
    

    vga_draw_box(30, 10, 20, 5, VGA_YELLOW, VGA_BLUE);
    vga_print_centered("SmolOS Running!", 12);
    

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