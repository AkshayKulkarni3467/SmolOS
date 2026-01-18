#include "sos_cmdshell.h"
#include "sos_cmds.h"
#include "sos_fat16.h"
#include "sos_vga.h"



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
    shell_check_scroll();
}

void cmd_color(CommandArgs args) {
    if (args.argc < 3) {
        print_with_scroll("Usage: color <foreground> <background>");
        print_with_scroll("Colors: 0-15 (0=black, 7=grey, 15=white)");
        return;
    }
    
    int fg = args.args[1][0] - '0';
    int bg = args.args[2][0] - '0';
    
    if (fg < 0 || fg > 15 || bg < 0 || bg > 15) {
        shell_print_error("Colors must be 0-15");
        return;
    }
    
    vga_set_color(fg, bg);
    for(int i = 0;i<WIDTH;i++){
        for(int j = 0;j<HEIGHT;j++){
            vga_putchr_at(i,j,' ');
        }
    }
    shell_print_success("Color changed!");
}

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
    
    int cur_x, cur_y;
    vga_get_cursor_pos(&cur_x, &cur_y);
    
    vga_rainbow_text(text, cur_x, cur_y);
    vga_println("");
    shell_check_scroll();
}

void cmd_calc(CommandArgs args) {
    if (args.argc < 4) {
        print_with_scroll("Usage: calc <num1> <op> <num2>");
        print_with_scroll("Operators: + - * /");
        print_with_scroll("Example: calc 10 + 5");
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
    shell_check_scroll();
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
        print_with_scroll(shell_get_username());
        return;
    }
    
    shell_set_username(args.args[1]);
    shell_print_success("Username updated!");
}



void cmd_clear(CommandArgs args) {
    vga_clear();
}