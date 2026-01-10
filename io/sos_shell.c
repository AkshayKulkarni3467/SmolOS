
#include "sos_shell.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_string.h"
#include "sos_cmds.h"

#define INPUT_BUFFER 256
#define HISTORY_SIZE 20

static char command_history[HISTORY_SIZE][INPUT_BUFFER];
static int history_count = 0;
static int history_index = 0;

static char current_input[INPUT_BUFFER];
static int cursor_pos = 0;

static uint8_t prompt_color = VGA_LGREEN;
static uint8_t input_color = VGA_WHITE;
static uint8_t error_color = VGA_LRED;
static uint8_t success_color = VGA_LCYAN;

static int shell_running = 1;
static char current_directory[64] = "/";
static char username[32] = "user";

int scroll_enabled = 1;

void shell_init(void) {
    history_count = 0;
    history_index = 0;
    cursor_pos = 0;
    shell_running = 1;
    strcpy(current_directory, "/");
    strcpy(username, "user");
    scroll_enabled = 1;
}

void shell_set_prompt_color(uint8_t color) {
    prompt_color = color;
}

void shell_set_input_color(uint8_t color) {
    input_color = color;
}

char* shell_get_directory(void) {
    return current_directory;
}

char* shell_get_username(void) {
    return username;
}

void shell_set_directory(const char* dir) {
    strncpy(current_directory, (char*)dir, 63);
    current_directory[63] = '\0';
}

void shell_set_username(const char* name) {
    strncpy(username, (char*)name, 31);
    username[31] = '\0';
}

void shell_add_to_history(const char* cmd) {
    if (cmd[0] == '\0') return;
    
    if (history_count > 0 && strcmp(command_history[history_count - 1], (char*)cmd) == 0) {
        return;
    }
    
    if (history_count < HISTORY_SIZE) {
        strcpy(command_history[history_count], (char*)cmd);
        history_count++;
    } else {
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            strcpy(command_history[i], command_history[i + 1]);
        }
        strcpy(command_history[HISTORY_SIZE - 1], (char*)cmd);
    }
    
    history_index = history_count;
}

void shell_prompt(void) {
    vga_set_color(prompt_color, VGA_BLCK);
    vga_print(username);
    vga_print("@SmolOS");
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print(":");
    
    vga_set_color(VGA_LBLUE, VGA_BLCK);
    vga_print(current_directory);
    
    vga_set_color(prompt_color, VGA_BLCK);
    vga_print("$ ");
    
    vga_set_color(input_color, VGA_BLCK);
}

void shell_display_startup(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    for (int i = 0; startup_text[i] != 0; i++) {
        vga_println((char*)startup_text[i]);
    }
    
    vga_println("");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("==============================================", 8);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_centered("Welcome to SmolOS - A Minimal Operating System", 9);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("==============================================", 10);
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("\n");
    vga_println("Type 'help' for a list of commands.");
    vga_println("Type 'about' for system information.");
    vga_println("");
    
    vga_end_batch();
}

void shell_clear_input_line(void) {
    int prompt_len = strlen(username) + strlen(current_directory) + 11; 
    
    for (int i = 0; i < cursor_pos; i++) {
        vga_putchr('\b');
        vga_putchr(' ');
        vga_putchr('\b');
    }
}

void shell_redraw_input(void) {
    vga_set_color(input_color, VGA_BLCK);
    for (int i = 0; i < cursor_pos; i++) {
        vga_putchr(current_input[i]);
    }
}

void shell_handle_history_up(void) {
    if (history_count == 0) return;
    
    if (history_index > 0) {
        history_index--;
        
        shell_clear_input_line();
        
        strcpy(current_input, command_history[history_index]);
        cursor_pos = strlen(current_input);
        
        shell_redraw_input();
    }
}

void shell_handle_history_down(void) {
    if (history_count == 0) return;
    
    if (history_index < history_count - 1) {
        history_index++;
        
        shell_clear_input_line();
        
        strcpy(current_input, command_history[history_index]);
        cursor_pos = strlen(current_input);
        
        shell_redraw_input();
    } else if (history_index == history_count - 1) {
        history_index = history_count;
        
        shell_clear_input_line();
        current_input[0] = '\0';
        cursor_pos = 0;
    }
}

void shell_autocomplete(void) {
    const char* commands[] = {
        "help", "clear", "about", "echo", "calc", "time",
        "uptime", "color", "username", "history","exit", 
        "sysinfo", "version", "banner", "test", "files"
    };
    int num_commands = 16;
    
    if (cursor_pos == 0) return;
    
    int matches = 0;
    int match_index = -1;
    
    for (int i = 0; i < num_commands; i++) {
        if (strstartswith((char*)commands[i], current_input)) {
            matches++;
            match_index = i;
        }
    }
    
    if (matches == 1) {
        shell_clear_input_line();
        strcpy(current_input, (char*)commands[match_index]);
        cursor_pos = strlen(current_input);
        shell_redraw_input();
    } else if (matches > 1) {
        vga_putchr('\n');
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_println("Available commands:");
        
        for (int i = 0; i < num_commands; i++) {
            if (strstartswith((char*)commands[i], current_input)) {
                vga_print("  ");
                vga_println((char*)commands[i]);
            }
        }
        
        shell_check_scroll();
        
        shell_prompt();
        shell_redraw_input();
    }
}

void shell_check_scroll(void) {
    if (!scroll_enabled) return;
    
    int current_row, current_col;
    vga_get_cursor_pos(&current_col, &current_row);
    
    if (current_row >= HEIGHT - 1) {
        vga_scroll();
        vga_t_row = HEIGHT - 2;
        vga_t_column = 0;
        vga_setcursor(0, HEIGHT - 2);
    }
}

int shell_is_running(void) {
    return shell_running;
}

void shell_exit(void) {
    shell_running = 0;
}

void run_shell(void) {
    shell_init();
    shell_display_startup();
    shell_prompt();
    
    cursor_pos = 0;
    current_input[0] = '\0';
    
    while (shell_running) {
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            
            if (c == '\n') {
                current_input[cursor_pos] = '\0';
                vga_putchr('\n');
                
                shell_check_scroll();
                
                if (cursor_pos > 0) {
                    shell_add_to_history(current_input);
                    run_command(current_input);
                }
                
                shell_check_scroll();
                
                shell_prompt();
                cursor_pos = 0;
                current_input[0] = '\0';
            }
            else if (c == '\b') {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    current_input[cursor_pos] = '\0';
                    vga_backspace();
                }
            }
            else if (c == '\t') {
                shell_autocomplete();
            }
            else if (c == CHAR_UP) {
                shell_handle_history_up();
            }
            else if (c == CHAR_DOWN) {
                shell_handle_history_down();
            }
            else if (c == CHAR_LEFT || c == CHAR_RIGHT) {
                //TODO Implement cursor movement
            }
            else if (c == CHAR_HOME) {
                shell_clear_input_line();
                cursor_pos = 0;
            }
            else if (is_printable(c)) {
                if (cursor_pos < INPUT_BUFFER - 1) {
                    current_input[cursor_pos++] = c;
                    current_input[cursor_pos] = '\0';
                    vga_putchr(c);
                }
            }
            else if (c == 3) {
                vga_putchr('\n');
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_println("^C");
                vga_set_color(input_color, VGA_BLCK);
                shell_check_scroll();
                shell_prompt();
                cursor_pos = 0;
                current_input[0] = '\0';
            }
            else if (c == 12) {
                vga_clear();
                shell_prompt();
                shell_redraw_input();
            }
        }
        
        for (volatile int i = 0; i < 10000; i++) {
            asm volatile ("nop");
        }
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("");
    vga_println("Shell terminated. System halted.");
    vga_println("Press reset to restart.");
}


void shell_print_error(const char* msg) {
    vga_set_color(error_color, VGA_BLCK);
    vga_print("Error: ");
    vga_println((char*)msg);
    vga_set_color(input_color, VGA_BLCK);
    shell_check_scroll();
}

void shell_print_success(const char* msg) {
    vga_set_color(success_color, VGA_BLCK);
    vga_println((char*)msg);
    vga_set_color(input_color, VGA_BLCK);
    shell_check_scroll();
}

void shell_print_info(const char* msg) {
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println((char*)msg);
    vga_set_color(input_color, VGA_BLCK);
    shell_check_scroll();
}

void shell_show_history(void) {
    if (history_count == 0) {
        vga_println("No command history.");
        shell_check_scroll();
        return;
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("Command History:");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    for (int i = 0; i < history_count; i++) {
        vga_print("  ");
        vga_print_int(i + 1);
        vga_print("  ");
        vga_println(command_history[i]);
        shell_check_scroll();
    }
}

void shell_clear_history(void) {
    history_count = 0;
    history_index = 0;
    shell_print_success("Command history cleared.");
}