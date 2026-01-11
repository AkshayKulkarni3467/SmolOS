#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_shell.h"
#include "sos_fat16.h"
#include "sos_rtc.h"
#include "sos_idt.h"

void kernel_main(void) {
    vga_init();
    keyboard_init();
    idt_init();
    pit_init(PIT_FREQ_18HZ);
    rtc_init();
    rtc_start_uptime();
    fat16_init();
    run_shell();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}