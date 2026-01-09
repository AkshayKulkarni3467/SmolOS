#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_shell.h"


void kernel_main(void) {
    vga_init();  
    keyboard_init();
    
    run_shell();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}