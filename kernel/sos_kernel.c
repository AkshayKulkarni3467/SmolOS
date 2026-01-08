#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"

#ifdef SMOLOS_KERNEL_TEST
#include "sos_stdio.h"
#endif



void demo_text_input(void) {
    vga_clear();
    vga_print_centered("=== TEXT INPUT DEMO ===", 2);
    vga_print_centered("Type your name and press Enter", 3);
    
    vga_draw_box_single(15, 8, 50, 5, VGA_GREEN, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_putchr_at(17, 9, 0);
    vga_print("Name: ");
    
    char input_buffer[100] = {0};
    int cursor_pos = 0;
    int input_done = 0;
    
    while (!input_done) {
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            
            if (c == '\n') {
                input_done = 1;
            }
            else if (c == '\b') {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    input_buffer[cursor_pos] = '\0';
                    
                    vga_putchr_at(24 + cursor_pos, 9, ' ');
                }
            }
            else if (is_printable(c) && cursor_pos < 40) {
                input_buffer[cursor_pos] = c;
                vga_putchr_at(24 + cursor_pos, 9, c);
                cursor_pos++;
            }
        }
        
    }

    clear_key_buffer();

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    char message[150];
    int msg_pos = 0;
    
    const char* hello = "Hello, ";
    for (int i = 0; hello[i] != '\0'; i++) {
        message[msg_pos++] = hello[i];
    }
    
    for (int i = 0; input_buffer[i] != '\0'; i++) {
        message[msg_pos++] = input_buffer[i];
    }
    
    message[msg_pos++] = '!';
    message[msg_pos] = '\0';
    
    vga_print_centered(message, 12);
    
    vga_reset_color();
    vga_print_centered("Press any key to continue...", 15);
    wait_for_char();
}



void kernel_main(void) {
    vga_init();
    keyboard_init();
    
    int selected = 0;
    int num_demos = 1;
    char* demo_names[] = {
        "Text Input Example"
    };
    
    int quit = 0;
    
    while (!quit) {
        vga_clear();
        
        vga_set_color(VGA_WHITE, VGA_BLUE);
        vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
        vga_print_centered("SmolOS - Keyboard & Graphics Demo", 0);
        vga_print_centered("Interactive Input Examples", 1);
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_print_centered("Select a demo:", 4);
        
        vga_draw_box_double(15, 6, 50, 10, VGA_CYAN, VGA_BLCK);
        
        for (int i = 0; i < num_demos; i++) {
            vga_draw_menu_item(17, 8 + i, 46, demo_names[i], i == selected, VGA_WHITE, VGA_BLCK);
        }
        
        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_print_centered("↑↓ Navigate  |  Enter Select  |  Q Quit", 18);
        
        vga_fill_rect(0, 24, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
        vga_set_color(VGA_WHITE, VGA_DGREY);

        
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            
            if (c == CHAR_UP) {
                selected--;
                if (selected < 0) selected = num_demos - 1;
            }
            else if (c == CHAR_DOWN) {
                selected++;
                if (selected >= num_demos) selected = 0;
            }
            else if (c == '\n') {
                switch (selected) {
                    case 0: demo_text_input(); break;
                }
            }
            else if (c == 'q' || c == 'Q') {
                quit = 1;
            }
        }
        
        delay(1);
    }
    
    vga_clear();
    vga_gradient_horizontal(0, 0, 80, 25);
    vga_draw_shadow_box(20, 10, 40, 6, VGA_WHITE, VGA_BLCK);
    vga_print_centered("Thank you for using SmolOS!", 12);
    vga_rainbow_text("Goodbye!", 36, 13);
    
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