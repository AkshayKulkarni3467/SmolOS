// kernel/sos_kernel_flicker_free.c
// Demo showing before/after double buffering

#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"

void demo_complex_ui(void) {
    vga_enable_double_buffer(1);
    
    int selected = 0;
    char* items[] = {"System", "Display", "Network", "Advanced", "About"};
    int progress = 0;
    
    for (int frame = 0; frame < 2000; frame++) {
        vga_begin_batch();
        
        vga_clear();
        
        vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
        vga_set_color(VGA_WHITE, VGA_BLUE);
        vga_print_centered("SmolOS Settings", 0);
        vga_print_centered("Settings example!", 1);
        
        vga_draw_window(10, 3, 60, 15, "Configuration", VGA_WHITE, VGA_BLCK);
        
        for (int i = 0; i < 5; i++) {
            vga_draw_menu_item(12, 6 + i, 26, items[i], i == selected, VGA_WHITE, VGA_BLCK);
        }
        
        vga_draw_box_single(40, 5, 28, 11, VGA_DGREY, VGA_BLCK);
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_print_centered("Current Status", 6);
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_putchr_at(42, 8, 0);
        vga_print("Selected: ");
        vga_set_color(VGA_CYAN, VGA_BLCK);
        vga_print(items[selected]);
        
        progress = (progress + 2) % 101;
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_putchr_at(42, 10, 0);
        vga_print("Loading:");
        vga_draw_progress_bar(42, 11, 24, progress, VGA_GREEN, VGA_DGREY, VGA_BLCK);
        
        vga_putchr_at(42, 13, 0);
        vga_print("Processing ");
        vga_draw_spinner(54, 13, frame, VGA_CYAN, VGA_BLCK);
        
        vga_fill_rect(0, 24, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
        vga_set_color(VGA_WHITE, VGA_DGREY);
        vga_putchr_at(2, 24, 0);
        vga_print("Frame: ");
        vga_print_int(frame);
        vga_print("  Use up/down arrows  |  ESC to exit");
        
        vga_end_batch();
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            if (c == CHAR_UP) {
                selected = (selected > 0) ? selected - 1 : 4;
            }
            else if (c == CHAR_DOWN) {
                selected = (selected < 4) ? selected + 1 : 0;
            }
            else if (c == 27) {  
                break;
            }
        }
        
        delay(1);
    }
}


void kernel_main(void) {
    vga_init();  
    keyboard_init();
    
    vga_begin_batch();
    vga_clear();
    vga_gradient_horizontal(0, 0, 80, 25);
    
    vga_draw_shadow_box(15, 8, 50, 10, VGA_WHITE, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLUE);
    vga_fill_rect(16, 9, 48, 1, ' ', VGA_WHITE, VGA_BLUE);
    vga_print_centered("SmolOS GUI", 9);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_centered("Welcome to SmolOS GUI!", 12);
    vga_print_centered("Press any key to start...", 14);
    vga_end_batch();
    
    wait_for_char();
    
    demo_complex_ui();

    
    vga_begin_batch();
    vga_clear();
    vga_print_centered("Bye!", 10);
    vga_rainbow_text("Blah blah blah blah!", 30, 12);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_centered("Blah blah blah OS blah blah blah!", 14);
    vga_end_batch();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}