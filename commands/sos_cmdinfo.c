#include "sos_cmdinfo.h"
#include "sos_cmds.h"
#include "sos_vga.h"
#include "sos_shell.h"

void cmd_about(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    for (int i = 0; startup_text[i] != 0; i++) {
        print_with_scroll((char*)startup_text[i]);
    }
    
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll("SmolOS Version 1.0");
    print_with_scroll("A minimal bare-metal operating system");
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll("Features:");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  * VGA Text Mode Graphics");
    print_with_scroll("  * Keyboard Input Support");
    print_with_scroll("  * Double-Buffered Rendering");
    print_with_scroll("  * Command Shell Interface");
    print_with_scroll("");
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("Created by: Akshay");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_version(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("SmolOS Version 1.0.0");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("Build: January 2026");
    print_with_scroll("Architecture: x86 (32-bit)");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_banner(CommandArgs args) {
    int save_row = vga_t_row;
    
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(10, 3, 60, 15, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    int start_line = 5;
    for (int i = 0; startup_text[i] != 0; i++) {
        vga_print_centered((char*)startup_text[i], start_line + i);
    }
    
    vga_print_centered("Press any key to continue...", 19);
    
    vga_end_batch();
    
    wait_for_char();
    vga_clear();
}

void cmd_sysinfo(CommandArgs args) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_single(5, 2, 70, 18, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("System Information", 3);
    
    vga_draw_separator(6, 4, 68, VGA_CYAN, VGA_BLCK);

    print_at_pos(8, 6, "OS Name:        ", VGA_YELLOW);
    print_at_pos(24, 6, "SmolOS v1.0", VGA_WHITE);
    
    print_at_pos(8, 7, "Architecture:   ", VGA_YELLOW);
    print_at_pos(24, 7, "x86 (32-bit)", VGA_WHITE);
    
    print_at_pos(8, 8, "Boot Mode:      ", VGA_YELLOW);
    print_at_pos(24, 8, "Multiboot", VGA_WHITE);
    
    print_at_pos(8, 9, "Display:        ", VGA_YELLOW);
    print_at_pos(24, 9, "VGA Text Mode (80x25)", VGA_WHITE);
    
    print_at_pos(8, 10, "Memory (Stack): ", VGA_YELLOW);
    print_at_pos(24, 10, "8 KB", VGA_WHITE);
    
    print_at_pos(8, 11, "Graphics:       ", VGA_YELLOW);
    print_at_pos(24, 11, "Double Buffered", VGA_WHITE);
    
    print_at_pos(8, 12, "Keyboard:       ", VGA_YELLOW);
    print_at_pos(24, 12, "PS/2 Compatible", VGA_WHITE);
    
    print_at_pos(8, 13, "Shell:          ", VGA_YELLOW);
    print_at_pos(24, 13, "SmolOS v1.0", VGA_WHITE);
    
    vga_draw_separator(6, 15, 68, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_GREEN, VGA_BLCK);
    vga_print_centered("System Status: Running", 17);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Press any key to continue...", 19);
    
    vga_end_batch();
    
    wait_for_char();
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}
