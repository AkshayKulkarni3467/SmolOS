#include "sos_shell.h"
#include "sos_vga.h"
#include "sos_keyboard.h"
#include "sos_cmds.h"


#ifdef SMOLOS_SHELL_TEST
#include "sos_stdio.h"
#endif

#define INPUT_BUFFER 128

static void shell_prompt(void) {
    vga_print("SmolOS >> ");
}

void run_shell(void) {
    char input[INPUT_BUFFER];
    int pos = 0;

    vga_clear();


    for (int i = 0; startup_text[i] != 0; i++) {
        for (int j = 0; startup_text[i][j] != '\0'; j++) {
            vga_putchr(startup_text[i][j]);
        }
        vga_putchr('\n');
    }

    vga_println("Welcome to SmolOS! Type 'help' for commands.");
        
    shell_prompt();

    while (1) {
                
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == '\n') {
                input[pos] = '\0';
                vga_putchr('\n');
                run_command(input);
                shell_prompt();
                pos = 0;
            }
            else if (c == '\b') {
                if (pos > 0) {
                    pos--;
                    vga_putchr('\b');
                }
            }
            else {
                if (pos < INPUT_BUFFER - 1) {
                    input[pos++] = c;
                    vga_putchr(c);
                }
            }
        }
        
        for (volatile int i = 0; i < 10000; i++) {
            asm volatile ("nop");
        }
    }
}

#ifdef SMOLOS_SHELL_TEST
int main(void){
    printf("Hello from Shell!\n");
}
#endif