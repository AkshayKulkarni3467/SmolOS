#include "sos_cmdgui.h"
#include "sos_cmds.h"
#include "sos_gui.h"
#include "sos_vga.h"
#include "sos_musicplayer.h"


void cmd_gui(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("Starting SmolOS GUI...");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    pit_delay_ms(500);
    
    gui_init();
    gui_run();
    gui_shutdown();
    
    vga_clear();
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    print_with_scroll("GUI closed. Welcome back to shell!");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_musicplayer(CommandArgs args){
    audio_demo_run();
}

void cmd_files(CommandArgs args) {
    file_manager_command();
}
