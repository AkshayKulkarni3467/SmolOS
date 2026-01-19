    #include "sos_vga.h"
    #include "sos_vgraphics.h"
    #include "sos_keyboard.h"
    #include "sos_pit.h"
    #include "sos_shell.h"
    #include "sos_fat16.h"
    #include "sos_rtc.h"
    #include "sos_idt.h"
    #include "sos_pci.h"
    #include "sos_net.h"
    #include "sos_mouse.h"
    #include "sos_mousecalib.h"

void kernel_main(unsigned long magic, unsigned long addr) {
    (void)magic;  
    (void)addr;
    vga_init();
    keyboard_init();
    idt_init();
    pit_init(PIT_FREQ_18HZ);  
    rtc_init();
    rtc_start_uptime();
    pci_init();
    fat16_init();
    
    mouse_init();
    mouse_hide_cursor();
    
    mouse_calibration_init();
    
    if (mouse_needs_calibration()) {
        mouse_run_calibration();
    }
    
    mouse_hide_cursor();
    
    audio_init();
    cmd_net_init();
    
    run_shell();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}