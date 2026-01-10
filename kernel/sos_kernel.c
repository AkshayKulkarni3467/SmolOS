#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_shell.h"
#include "sos_fat16.h"


void kernel_main(void) {
    vga_init();  
    keyboard_init();
    fat16_init();
    run_shell();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}