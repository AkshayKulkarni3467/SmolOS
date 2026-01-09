
#include "sos_cmds.h"
#include "sos_vga.h"
#include "sos_shell.h"
void run_command(const char* cmd) {
    
    if (cmd[0] == '\0') {
        return;
    }

    if (strcmp(cmd, "help") == 0) {
        vga_println("Commands: help, clear, about, calc, clock, info");
        vga_println("See the guide for all commands....");
        vga_println("  help  - Show this help message");
        vga_println("  clear - Clear the screen");
        vga_println("  about - Show system information");
    } 
    else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    } 
    else if (strcmp(cmd, "about") == 0) {
        for (int i = 0; startup_text[i] != 0; i++) {
            vga_println(startup_text[i]);
        }
        vga_println("SmolOS V1.0 - BareMetal OS by Akshay");
    }
    else {
        vga_print("Unknown command: ");
        vga_println(cmd);
        vga_println("Type 'help' for available commands");
    }
}
