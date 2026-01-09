#include "sos_cmds.h"
#include "sos_shell.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_string.h"
#include "sos_memory.h"

#define MAX_ARGS 10

typedef struct {
    char* args[MAX_ARGS];
    int argc;
} CommandArgs;

static CommandArgs parse_command(char* cmd) {
    CommandArgs result;
    result.argc = 0;
    
    char* token = cmd;
    int in_word = 0;
    
    for (int i = 0; cmd[i] != '\0' && result.argc < MAX_ARGS; i++) {
        if (cmd[i] == ' ' || cmd[i] == '\t') {
            if (in_word) {
                cmd[i] = '\0';
                in_word = 0;
            }
        } else {
            if (!in_word) {
                result.args[result.argc++] = &cmd[i];
                in_word = 1;
            }
        }
    }
    
    return result;
}

void cmd_help(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("=== SmolOS Command Reference ===");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("System Commands:");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("  help           - Show this help message");
    vga_println("  about          - Display system information");
    vga_println("  version        - Show SmolOS version");
    vga_println("  sysinfo        - Display detailed system info");
    vga_println("  clear          - Clear the screen");
    vga_println("  exit           - Exit the shell");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Display Commands:");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("  echo <text>    - Print text to screen");
    vga_println("  color <fg> <bg>- Change text color");
    vga_println("  banner         - Display SmolOS banner");
    vga_println("  rainbow <text> - Print text in rainbow colors");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Shell Commands:");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("  history        - Show command history");
    vga_println("  clear-history  - Clear command history");
    vga_println("  username <name>- Set username");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Utility Commands:");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("  calc <expr>    - Simple calculator");
    vga_println("  time           - Show current time (simulated)");
    vga_println("  uptime         - Show system uptime");
    vga_println("  test           - Run graphics test");
    vga_println("");
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_println("Tip: Use TAB for autocomplete, UP/DOWN for history");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_about(CommandArgs args) {
    vga_begin_batch();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    for (int i = 0; startup_text[i] != 0; i++) {
        vga_println((char*)startup_text[i]);
    }
    
    vga_println("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("SmolOS Version 1.0");
    vga_println("A minimal bare-metal operating system");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Features:");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("  * VGA Text Mode Graphics");
    vga_println("  * Keyboard Input Support");
    vga_println("  * Double-Buffered Rendering");
    vga_println("  * Command Shell Interface");
    vga_println("");
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("Created by: Akshay");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    vga_end_batch();
}

void cmd_version(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("SmolOS Version 1.0.0");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_println("Build: January 2026");
    vga_println("Architecture: x86 (32-bit)");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_clear(CommandArgs args) {
    vga_clear();
}

void cmd_echo(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: echo <text>");
        return;
    }
    
    for (int i = 1; i < args.argc; i++) {
        vga_print(args.args[i]);
        if (i < args.argc - 1) vga_print(" ");
    }
    vga_println("");
}

//TODO Fix function bug
void cmd_color(CommandArgs args) {
    if (args.argc < 3) {
        vga_println("Usage: color <foreground> <background>");
        vga_println("Colors: 0-15 (0=black, 7=grey, 15=white)");
        return;
    }
    
    //TODO Implement atoi(..) function
    
    int fg = args.args[1][0] - '0';
    int bg = args.args[2][0] - '0';
    
    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        shell_print_error("Colors must be 0-15");
        return;
    }
    
    vga_set_color(fg, bg);
    shell_print_success("Color changed!");
}

void cmd_banner(CommandArgs args) {
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


//TODO Font appears at 0,0; will fix later
void cmd_rainbow(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: rainbow <text>");
        return;
    }
    
    char text[256] = {0};
    int pos = 0;
    
    for (int i = 1; i < args.argc; i++) {
        int j = 0;
        while (args.args[i][j] && pos < 255) {
            text[pos++] = args.args[i][j++];
        }
        if (i < args.argc - 1 && pos < 255) {
            text[pos++] = ' ';
        }
    }
    text[pos] = '\0';
    
    vga_rainbow_text(text, 0, vga_t_row);
    vga_println("");
}

void cmd_sysinfo(CommandArgs args) {
    vga_begin_batch();
    
    vga_draw_box_single(5, 2, 70, 18, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("System Information", 3);
    
    vga_draw_separator(6, 4, 68, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 6, 0);
    vga_print("OS Name:        ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("SmolOS v1.0");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 7, 0);
    vga_print("Architecture:   ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("x86 (32-bit)");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 8, 0);
    vga_print("Boot Mode:      ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("Multiboot");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 9, 0);
    vga_print("Display:        ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("VGA Text Mode (80x25)");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 10, 0);
    vga_print("Memory (Stack): ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("8 KB");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 11, 0);
    vga_print("Graphics:       ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("Double Buffered");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 12, 0);
    vga_print("Keyboard:       ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("PS/2 Compatible");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(8, 13, 0);
    vga_print("Shell:          ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("SmolShell v1.0");
    
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

void cmd_calc(CommandArgs args) {
    if (args.argc < 4) {
        vga_println("Usage: calc <num1> <op> <num2>");
        vga_println("Operators: + - * /");
        vga_println("Example: calc 10 + 5");
        return;
    }
    
    int num1 = 0, num2 = 0;
    
    int i = 0;
    while (args.args[1][i]) {
        if (args.args[1][i] >= '0' && args.args[1][i] <= '9') {
            num1 = num1 * 10 + (args.args[1][i] - '0');
        }
        i++;
    }
    
    i = 0;
    while (args.args[3][i]) {
        if (args.args[3][i] >= '0' && args.args[3][i] <= '9') {
            num2 = num2 * 10 + (args.args[3][i] - '0');
        }
        i++;
    }
    
    char op = args.args[2][0];
    int result = 0;
    int valid = 1;
    
    switch (op) {
        case '+': result = num1 + num2; break;
        case '-': result = num1 - num2; break;
        case '*': result = num1 * num2; break;
        case '/':
            if (num2 == 0) {
                shell_print_error("Division by zero!");
                return;
            }
            result = num1 / num2;
            break;
        default:
            shell_print_error("Invalid operator. Use: + - * /");
            return;
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("Result: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_int(num1);
    vga_print(" ");
    vga_putchr(op);
    vga_print(" ");
    vga_print_int(num2);
    vga_print(" = ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_print_int(result);
    vga_println("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_time(CommandArgs args) {

    //TODO Implement real time
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("Current Time: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("12:34:56");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_uptime(CommandArgs args) {
    //TODO Implement real uptime
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("System Uptime: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("0 days, 0 hours, 5 minutes");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_history(CommandArgs args) {
    shell_show_history();
}

void cmd_clear_history(CommandArgs args) {
    shell_clear_history();
}

void cmd_username(CommandArgs args) {
    if (args.argc < 2) {
        vga_print("Current username: ");
        vga_println(shell_get_username());
        return;
    }
    
    shell_set_username(args.args[1]);
    shell_print_success("Username updated!");
}

void cmd_exit(CommandArgs args) {
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Exiting shell...");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    shell_exit();
}

void cmd_test(CommandArgs args) {
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Graphics Demo", 2);
    
    vga_draw_box_single(10, 5, 20, 5, VGA_RED, VGA_BLCK);
    vga_draw_box_double(35, 5, 20, 5, VGA_GREEN, VGA_BLCK);
    vga_draw_shadow_box(60, 5, 15, 5, VGA_BLUE, VGA_BLCK);
    
    vga_draw_progress_bar(10, 12, 30, 75, VGA_GREEN, VGA_DGREY, VGA_BLCK);
    vga_draw_progress_bar(10, 14, 30, 50, VGA_YELLOW, VGA_DGREY, VGA_BLCK);
    vga_draw_progress_bar(10, 16, 30, 25, VGA_RED, VGA_DGREY, VGA_BLCK);
    
    vga_rainbow_text("SmolOS Graphics!", 25, 19);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Press any key to continue...", 22);
    
    vga_end_batch();
    
    wait_for_char();
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


void run_command(const char* cmd_str) {
    if (cmd_str[0] == '\0') {
        return;
    }
    
    char cmd_copy[256];
    strcpy(cmd_copy, (char*)cmd_str);
    
    CommandArgs args = parse_command(cmd_copy);
    
    if (args.argc == 0) return;
    
    char* cmd = args.args[0];
    
    if (strcmp(cmd, "help") == 0) {
        cmd_help(args);
    }
    else if (strcmp(cmd, "about") == 0) {
        cmd_about(args);
    }
    else if (strcmp(cmd, "version") == 0) {
        cmd_version(args);
    }
    else if (strcmp(cmd, "clear") == 0) {
        cmd_clear(args);
    }
    else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    }
    else if (strcmp(cmd, "color") == 0) {
        cmd_color(args);
    }
    else if (strcmp(cmd, "banner") == 0) {
        cmd_banner(args);
    }
    else if (strcmp(cmd, "rainbow") == 0) {
        cmd_rainbow(args);
    }
    else if (strcmp(cmd, "sysinfo") == 0) {
        cmd_sysinfo(args);
    }
    else if (strcmp(cmd, "calc") == 0) {
        cmd_calc(args);
    }
    else if (strcmp(cmd, "time") == 0) {
        cmd_time(args);
    }
    else if (strcmp(cmd, "uptime") == 0) {
        cmd_uptime(args);
    }
    else if (strcmp(cmd, "history") == 0) {
        cmd_history(args);
    }
    else if (strcmp(cmd, "clear-history") == 0) {
        cmd_clear_history(args);
    }
    else if (strcmp(cmd, "username") == 0) {
        cmd_username(args);
    }
    else if (strcmp(cmd, "exit") == 0) {
        cmd_exit(args);
    }
    else if (strcmp(cmd, "test") == 0) {
        cmd_test(args);
    }
    else {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_print("Unknown command: '");
        vga_print(cmd);
        vga_println("'");
        vga_set_color(VGA_LGREY, VGA_BLCK);
        vga_println("Type 'help' for available commands");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    }
}