#include "sos_calculator.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_string.h"
#include "sos_memory.h"
#include "sos_pit.h"

#define MAX_INPUT 64
#define HISTORY_SIZE 10
#define PI 3.14159265358979323846
#define E  2.71828182845904523536

typedef struct {
    char display[MAX_INPUT];
    int display_pos;
    double memory;
    double last_result;
    char history[HISTORY_SIZE][MAX_INPUT];
    int history_count;
    int angle_mode; 
    int cursor_visible;
    int cursor_blink_counter;
    int show_result_highlight;
    int result_highlight_counter;
    int error_state;
    char error_message[64];
} CalcState;

static CalcState calc;

double power(double base, double exp) {
    if (exp == 0) return 1.0;
    if (exp < 0) return 1.0 / power(base, -exp);
    
    double result = 1.0;
    int int_exp = (int)exp;
    
    for (int i = 0; i < int_exp; i++) {
        result *= base;
    }
    
    if (exp != int_exp) {
        double frac = exp - int_exp;
        if (frac > 0.4 && frac < 0.6) {
            result *= sqrt(base);
        }
    }
    
    return result;
}

double sqrt(double x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    
    double guess = x / 2.0;
    double epsilon = 0.00001;
    
    for (int i = 0; i < 50; i++) {
        double new_guess = (guess + x / guess) / 2.0;
        if (new_guess > guess - epsilon && new_guess < guess + epsilon) {
            break;
        }
        guess = new_guess;
    }
    
    return guess;
}

double factorial(int n) {
    if (n < 0) return 0;
    if (n == 0 || n == 1) return 1;
    
    double result = 1;
    for (int i = 2; i <= n && i < 20; i++) {
        result *= i;
    }
    return result;
}

double sine(double x) {
    double term = x;
    double sum = term;
    
    for (int n = 1; n < 10; n++) {
        term *= -x * x / ((2 * n) * (2 * n + 1));
        sum += term;
    }
    
    return sum;
}

double cosine(double x) {
    double term = 1.0;
    double sum = term;
    
    for (int n = 1; n < 10; n++) {
        term *= -x * x / ((2 * n - 1) * (2 * n));
        sum += term;
    }
    
    return sum;
}

double tangent(double x) {
    double cos_x = cosine(x);
    if (cos_x > -0.0001 && cos_x < 0.0001) return 0; 
    return sine(x) / cos_x;
}

double logarithm(double x) {
    if (x <= 0) return 0;
    
    if (x > 0.5 && x < 1.5) {
        double y = (x - 1) / (x + 1);
        double y2 = y * y;
        double sum = 0;
        double term = y;
        
        for (int n = 0; n < 20; n++) {
            sum += term / (2 * n + 1);
            term *= y2;
        }
        
        return 2 * sum;
    } else {
        int exp = 0;
        double reduced = x;
        
        while (reduced > 1.5) {
            reduced /= E;
            exp++;
        }
        while (reduced < 0.5) {
            reduced *= E;
            exp--;
        }
        
        return logarithm(reduced) + exp;
    }
}

double deg_to_rad(double deg) {
    return deg * PI / 180.0;
}

double rad_to_deg(double rad) {
    return rad * 180.0 / PI;
}

double string_to_double(const char* str) {
    double result = 0.0;
    double fraction = 1.0;
    int sign = 1;
    int decimal_found = 0;
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    while (*str) {
        if (*str == '.') {
            if (decimal_found) break;
            decimal_found = 1;
        } 
        else if (*str >= '0' && *str <= '9') {
            if (decimal_found) {
                fraction *= 0.1;
                result += (*str - '0') * fraction;
            } else {
                result = result * 10.0 + (*str - '0');
            }
        }
        else if (*str == ' ' || *str == '\t') {

        }
        else {
            break;
        }
        str++;
    }
    
    return result * sign;
}

void double_to_string(double num, char* buffer) {
    if (num != num) { 
        strcpy(buffer, "Error: NaN");
        return;
    }
    if (num > 1e15 || num < -1e15) {
        strcpy(buffer, "Error: Overflow");
        return;
    }
    
    int sign = (num < 0) ? 1 : 0;
    if (sign) num = -num;
    
    int int_part = (int)num;
    double frac_part = num - int_part;
    
    int pos = 0;
    
    if (sign) buffer[pos++] = '-';
    
    if (int_part == 0) {
        buffer[pos++] = '0';
    } else {
        char temp[32];
        int temp_pos = 0;
        
        while (int_part > 0) {
            temp[temp_pos++] = '0' + (int_part % 10);
            int_part /= 10;
        }
        
        while (temp_pos > 0) {
            buffer[pos++] = temp[--temp_pos];
        }
    }
    
    if (frac_part > 0.000001) {
        buffer[pos++] = '.';
        
        for (int i = 0; i < 6; i++) {
            frac_part *= 10;
            int digit = (int)frac_part;
            buffer[pos++] = '0' + digit;
            frac_part -= digit;
        }
        
        pos--;
        while (pos > 0 && buffer[pos] == '0') {
            pos--;
        }
        if (buffer[pos] == '.') {
            pos--;
        }
        buffer[pos + 1] = '\0';
    } else {
        buffer[pos] = '\0';
    }
}

void add_to_history(const char* expression) {
    if (calc.history_count < HISTORY_SIZE) {
        strcpy(calc.history[calc.history_count], (char*)expression);
        calc.history_count++;
    } else {
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            strcpy(calc.history[i], calc.history[i + 1]);
        }
        strcpy(calc.history[HISTORY_SIZE - 1], (char*)expression);
    }
}

void print_at(int x, int y, const char* str, uint8_t fg, uint8_t bg) {
    int saved_x, saved_y;
    vga_save_cursor(&saved_x, &saved_y);
    
    vga_set_color(fg, bg);
    int i = 0;
    while (str[i] && x + i < 80) {
        vga_putchr_at(x + i, y, str[i]);
        i++;
    }
    
    vga_restore_cursor(saved_x, saved_y);
}

void draw_calculator_ui(void) {
    vga_begin_batch();
    vga_clear();
    
    int saved_x, saved_y;
    vga_save_cursor(&saved_x, &saved_y);
    
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
    print_at(15, 0, "SmolOS Scientific Calculator v2.0", VGA_YELLOW, VGA_BLUE);
    print_at(75, 0, "[X]", VGA_LRED, VGA_BLUE);
    
    vga_draw_box_double(2, 2, 76, 4, VGA_CYAN, VGA_BLCK);
    
    uint8_t display_bg = calc.show_result_highlight ? VGA_BLUE : VGA_DGREY;
    vga_fill_rect(3, 3, 74, 2, ' ', VGA_BLCK, display_bg);
    
    if (calc.error_state) {
        print_at(5, 3, "ERROR", VGA_LRED, display_bg);
        print_at(5, 4, calc.error_message, VGA_LRED, display_bg);
    } else {
        print_at(5, 4, calc.display, VGA_YELLOW, display_bg);
        
        if (calc.cursor_visible) {
            int cursor_x = 5 + strlen(calc.display);
            if (cursor_x < 77) {
                vga_putchr_at(cursor_x, 4, '_');
            }
        }
    }
    
    if (calc.memory != 0) {
        char mem_str[20];
        strcpy(mem_str, "M:");
        char mem_val[16];
        double_to_string(calc.memory, mem_val);
        strcat(mem_str, mem_val);
        print_at(68 - strlen(mem_str), 3, mem_str, VGA_LGREEN, display_bg);
    }
    
    print_at(70, 3, calc.angle_mode ? "RAD" : "DEG", VGA_CYAN, display_bg);
    
    print_at(2, 7, "Basic Operations:", VGA_LCYAN, VGA_BLCK);
    
    const char* buttons[][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"0", ".", "=", "+"}
    };
    
    const uint8_t button_colors[][4] = {
        {VGA_WHITE, VGA_WHITE, VGA_WHITE, VGA_YELLOW},
        {VGA_WHITE, VGA_WHITE, VGA_WHITE, VGA_YELLOW},
        {VGA_WHITE, VGA_WHITE, VGA_WHITE, VGA_YELLOW},
        {VGA_WHITE, VGA_WHITE, VGA_LGREEN, VGA_YELLOW}
    };
    
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int x = 4 + col * 8;
            int y = 9 + row * 2;
            
            vga_draw_box_single(x, y, 6, 2, VGA_LGREY, VGA_BLCK);
            print_at(x + 3, y + 1, buttons[row][col], button_colors[row][col], VGA_BLCK);
        }
    }
    
    vga_draw_box_single(37, 6, 42, 13, VGA_CYAN, VGA_BLCK);
    print_at(48, 6, " Scientific Functions ", VGA_YELLOW, VGA_BLCK);
    
    print_at(39, 8, "Trigonometric:", VGA_LCYAN, VGA_BLCK);
    print_at(39, 9, "  [S] sin    [C] cos    [T] tan", VGA_WHITE, VGA_BLCK);
    
    print_at(39, 11, "Advanced:", VGA_LCYAN, VGA_BLCK);
    print_at(39, 12, "  [Q] sqrt   [P] x^y    [L] ln", VGA_WHITE, VGA_BLCK);
    print_at(39, 13, "  [!] x!     [R] 1/x    [%] mod", VGA_WHITE, VGA_BLCK);
    
    print_at(39, 15, "Constants:", VGA_LCYAN, VGA_BLCK);
    print_at(39, 16, "  [A] π      [B] e", VGA_WHITE, VGA_BLCK);
    
    vga_draw_box_single(2, 17, 34, 4, VGA_GREEN, VGA_BLCK);
    print_at(10, 17, " Memory & Mode ", VGA_LGREEN, VGA_BLCK);
    print_at(4, 18, "[M] M+  [N] MR  [X] MC", VGA_WHITE, VGA_BLCK);
    print_at(4, 19, "[D] Toggle: ", VGA_WHITE, VGA_BLCK);
    print_at(16, 19, calc.angle_mode ? "Radians" : "Degrees ", VGA_YELLOW, VGA_BLCK);
    
    vga_draw_box_single(37, 19, 42, 4, VGA_MAGENTA, VGA_BLCK);
    print_at(48, 19, " Recent History ", VGA_LMAGENTA, VGA_BLCK);
    
    int start = (calc.history_count > 2) ? calc.history_count - 2 : 0;
    for (int i = start; i < calc.history_count && i < start + 2; i++) {
        char hist_line[80];
        int line_num = i - start;
        hist_line[0] = '0' + (i + 1);
        hist_line[1] = '.';
        hist_line[2] = ' ';
        hist_line[3] = '\0';
        strcat(hist_line, calc.history[i]);
        
        if (strlen(hist_line) > 38) {
            hist_line[35] = '.';
            hist_line[36] = '.';
            hist_line[37] = '.';
            hist_line[38] = '\0';
        }
        
        print_at(39, 20 + line_num, hist_line, VGA_LGREY, VGA_BLCK);
    }
    
    vga_fill_rect(0, 23, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
    print_at(2, 23, "F1:Help  F2:Clear  F3:Copy Result  ESC:Exit", VGA_YELLOW, VGA_BLUE);
    
    const char* tips[] = {
        "Tip: Use P for powers - enter base, press P, enter exponent",
        "Tip: Press D to toggle between Degrees and Radians",
        "Tip: Store values in memory with M, recall with N",
        "Tip: Chain calculations - result is saved automatically",
        "Tip: Use Q for square root, L for natural log"
    };
    int tip_index = (calc.display_pos + calc.history_count) % 5;
    print_at(2, 24, tips[tip_index], VGA_LGREY, VGA_BLUE);
    
    vga_restore_cursor(saved_x, saved_y);
    
    vga_end_batch();
}

void show_help(void) {
    vga_begin_batch();
    vga_clear();
    
    int saved_x, saved_y;
    vga_save_cursor(&saved_x, &saved_y);
    
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
    print_at(28, 0, "CALCULATOR HELP", VGA_YELLOW, VGA_BLUE);
    print_at(20, 1, "Complete Guide to Scientific Calculator", VGA_LGREY, VGA_BLUE);
    
    vga_draw_box_double(2, 3, 76, 18, VGA_CYAN, VGA_BLCK);
    
    print_at(5, 4, "BASIC OPERATIONS", VGA_LCYAN, VGA_BLCK);
    print_at(5, 5, "  Type numbers and operators: + - * /", VGA_WHITE, VGA_BLCK);
    print_at(5, 6, "  Supports operator precedence: 2+3*4 = 14 (not 20!)", VGA_YELLOW, VGA_BLCK);
    print_at(5, 7, "  Press ENTER or = to calculate", VGA_WHITE, VGA_BLCK);
    
    print_at(5, 9, "TRIGONOMETRIC FUNCTIONS", VGA_LCYAN, VGA_BLCK);
    print_at(5, 10, "  [S] sin(x)  [C] cos(x)  [T] tan(x)", VGA_WHITE, VGA_BLCK);
    print_at(5, 11, "  Note: Toggle DEG/RAD mode with [D] key", VGA_YELLOW, VGA_BLCK);
    
    print_at(5, 13, "ADVANCED FUNCTIONS", VGA_LCYAN, VGA_BLCK);
    print_at(5, 14, "  [Q] sqrt(x) - Square root      [L] ln(x)   - Natural log", VGA_WHITE, VGA_BLCK);
    print_at(5, 15, "  [!] x!      - Factorial        [R] 1/x     - Reciprocal", VGA_WHITE, VGA_BLCK);
    print_at(5, 16, "  [P] x^y     - Power (enter base, P, exponent)", VGA_WHITE, VGA_BLCK);
    print_at(5, 17, "  [%] x%y     - Modulo (remainder after division)", VGA_WHITE, VGA_BLCK);
    
    print_at(42, 9, "CONSTANTS", VGA_LCYAN, VGA_BLCK);
    print_at(42, 10, "  [A] π (3.14159...)", VGA_WHITE, VGA_BLCK);
    print_at(42, 11, "  [B] e (2.71828...)", VGA_WHITE, VGA_BLCK);
    
    print_at(42, 13, "MEMORY", VGA_LCYAN, VGA_BLCK);
    print_at(42, 14, "  [M] Memory Add (M+)", VGA_WHITE, VGA_BLCK);
    print_at(42, 15, "  [N] Memory Recall", VGA_WHITE, VGA_BLCK);
    print_at(42, 16, "  [X] Memory Clear", VGA_WHITE, VGA_BLCK);
    
    print_at(5, 19, "SHORTCUTS", VGA_LCYAN, VGA_BLCK);
    print_at(5, 20, "  [F1] This help    [F2] Clear display    [F3] Copy last result", VGA_WHITE, VGA_BLCK);
    
    vga_fill_rect(0, 22, 80, 3, ' ', VGA_WHITE, VGA_BLUE);
    print_at(22, 22, "Press any key to return to calculator", VGA_YELLOW, VGA_BLUE);
    print_at(15, 23, "Made with <3 for SmolOS - Advanced Edition", VGA_LGREY, VGA_BLUE);
    
    vga_restore_cursor(saved_x, saved_y);
    vga_end_batch();
    
    wait_for_char();
}

double evaluate_expression(const char* expr) {
    double numbers[32];
    char operators[32];
    int num_count = 0;
    int op_count = 0;
    
    int i = 0;
    int negative_next = 0;
    
    while (expr[i]) {
        while (expr[i] == ' ' || expr[i] == '\t') i++;
        
        if (expr[i] == '-' && (i == 0 || expr[i-1] == '+' || expr[i-1] == '-' || 
            expr[i-1] == '*' || expr[i-1] == '/' || expr[i-1] == '%')) {
            negative_next = 1;
            i++;
            continue;
        }
        
        if ((expr[i] >= '0' && expr[i] <= '9') || expr[i] == '.') {
            double num = 0;
            int decimal_found = 0;
            double decimal_place = 0.1;
            
            while ((expr[i] >= '0' && expr[i] <= '9') || expr[i] == '.') {
                if (expr[i] == '.') {
                    if (decimal_found) break;
                    decimal_found = 1;
                } else {
                    if (decimal_found) {
                        num += (expr[i] - '0') * decimal_place;
                        decimal_place *= 0.1;
                    } else {
                        num = num * 10 + (expr[i] - '0');
                    }
                }
                i++;
            }
            
            if (negative_next) {
                num = -num;
                negative_next = 0;
            }
            
            numbers[num_count++] = num;
        }
        else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/' || expr[i] == '%') {
            operators[op_count++] = expr[i];
            i++;
        }
        else if (expr[i] == '=' || expr[i] == '\0') {
            break;
        }
        else {
            i++;
        }
    }
    
    if (num_count == 0) return 0;
    if (num_count == 1) return numbers[0];
    
    for (int op_idx = 0; op_idx < op_count; op_idx++) {
        if (operators[op_idx] == '*' || operators[op_idx] == '/' || operators[op_idx] == '%') {
            double result;
            if (operators[op_idx] == '*') {
                result = numbers[op_idx] * numbers[op_idx + 1];
            } else if (operators[op_idx] == '/') {
                if (numbers[op_idx + 1] != 0) {
                    result = numbers[op_idx] / numbers[op_idx + 1];
                } else {
                    return 0; 
                }
            } else { 
                if (numbers[op_idx + 1] != 0) {
                    int a = (int)numbers[op_idx];
                    int b = (int)numbers[op_idx + 1];
                    result = a % b;
                } else {
                    return 0;
                }
            }
            
            numbers[op_idx] = result;
            
            for (int j = op_idx + 1; j < num_count - 1; j++) {
                numbers[j] = numbers[j + 1];
            }
            num_count--;
            
            for (int j = op_idx; j < op_count - 1; j++) {
                operators[j] = operators[j + 1];
            }
            op_count--;
            op_idx--; 
        }
    }
    
    double result = numbers[0];
    for (int op_idx = 0; op_idx < op_count; op_idx++) {
        if (operators[op_idx] == '+') {
            result += numbers[op_idx + 1];
        } else if (operators[op_idx] == '-') {
            result -= numbers[op_idx + 1];
        }
    }
    
    return result;
}

void show_calculation_animation(void) {
    for (int i = 0; i < 3; i++) {
        print_at(40, 4, ".", VGA_YELLOW, VGA_DGREY);
        delay(500);
        print_at(40, 4, "..", VGA_YELLOW, VGA_DGREY);
        delay(500);
        print_at(40, 4, "...", VGA_YELLOW, VGA_DGREY);
        delay(500);
        print_at(40, 4, "    ", VGA_YELLOW, VGA_DGREY);
    }
}

void calc_command(void) {
    memset(&calc, 0, sizeof(CalcState));
    calc.display[0] = '0';
    calc.display[1] = '\0';
    calc.display_pos = 1;
    calc.angle_mode = 0; 
    calc.cursor_visible = 1;
    calc.cursor_blink_counter = 0;
    calc.show_result_highlight = 0;
    calc.result_highlight_counter = 0;
    calc.error_state = 0;
    
    int running = 1;
    int loop_counter = 0;
    
    while (running) {
        loop_counter++;
        if (loop_counter % 3000 == 0) {
            calc.cursor_visible = !calc.cursor_visible;
            calc.cursor_blink_counter++;
        }
        
        if (calc.show_result_highlight) {
            calc.result_highlight_counter++;
            if (calc.result_highlight_counter > 15000) {
                calc.show_result_highlight = 0;
                calc.result_highlight_counter = 0;
            }
        }
        
        if (calc.error_state) {
            if (loop_counter % 30000 == 0) {
                calc.error_state = 0;
            }
        }
        
        if (loop_counter % 1000 == 0 || calc.cursor_blink_counter < 2) {
            draw_calculator_ui();
        }
        
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            calc.error_state = 0; 
            
            if (c == CHAR_F1) {
                show_help();
                loop_counter = 0;
            }
            else if (c == CHAR_F2) {
                calc.display[0] = '0';
                calc.display[1] = '\0';
                calc.display_pos = 1;
                calc.show_result_highlight = 0;
            }
            else if (c == CHAR_F3) {
                double_to_string(calc.last_result, calc.display);
                calc.display_pos = strlen(calc.display);
            }
            else if (c == 27) {
                running = 0;
            }
            else if (c == '\b') {
                if (calc.display_pos > 0) {
                    calc.display_pos--;
                    calc.display[calc.display_pos] = '\0';
                }
                if (calc.display_pos == 0) {
                    calc.display[0] = '0';
                    calc.display[1] = '\0';
                    calc.display_pos = 1;
                }
                calc.show_result_highlight = 0;
            }
            else if (c == '\n' || c == '=') {
                int has_number = 0;
                for (int i = 0; calc.display[i]; i++) {
                    if (calc.display[i] >= '0' && calc.display[i] <= '9') {
                        has_number = 1;
                        break;
                    }
                }
                
                if (!has_number) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Invalid expression");
                } else {
                    double result = evaluate_expression(calc.display);
                    
                    if (result != result) { 
                        calc.error_state = 1;
                        strcpy(calc.error_message, "Math error: NaN");
                    } else if (result > 1e15 || result < -1e15) {
                        calc.error_state = 1;
                        strcpy(calc.error_message, "Overflow error");
                    } else {
                        calc.last_result = result;
                        
                        char history_entry[MAX_INPUT];
                        strcpy(history_entry, calc.display);
                        strcat(history_entry, " = ");
                        char result_str[32];
                        double_to_string(result, result_str);
                        strcat(history_entry, result_str);
                        add_to_history(history_entry);
                        
                        double_to_string(result, calc.display);
                        calc.display_pos = strlen(calc.display);
                        
                        calc.show_result_highlight = 1;
                        calc.result_highlight_counter = 0;
                    }
                }
            }
            else if ((c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%') {
                if (calc.display_pos < MAX_INPUT - 1) {
                    if (calc.show_result_highlight && (c >= '0' && c <= '9')) {
                        calc.display[0] = c;
                        calc.display[1] = '\0';
                        calc.display_pos = 1;
                        calc.show_result_highlight = 0;
                    } else if (calc.display_pos == 1 && calc.display[0] == '0' && c != '.' && (c >= '0' && c <= '9')) {
                        calc.display[0] = c;
                        calc.display[1] = '\0';
                        calc.show_result_highlight = 0;
                    } else {
                        calc.display[calc.display_pos++] = c;
                        calc.display[calc.display_pos] = '\0';
                        calc.show_result_highlight = 0;
                    }
                }
            }
            else if (c == 's' || c == 'S') {
                double val = string_to_double(calc.display);
                if (!calc.angle_mode) val = deg_to_rad(val);
                double result = sine(val);
                if (result != result) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Math error");
                } else {
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 'c' || c == 'C') {
                double val = string_to_double(calc.display);
                if (!calc.angle_mode) val = deg_to_rad(val);
                double result = cosine(val);
                if (result != result) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Math error");
                } else {
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 't' || c == 'T') {
                double val = string_to_double(calc.display);
                if (!calc.angle_mode) val = deg_to_rad(val);
                double result = tangent(val);
                if (result != result) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Math error");
                } else {
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 'q' || c == 'Q') {
                double val = string_to_double(calc.display);
                if (val < 0) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Cannot sqrt negative number");
                } else {
                    double result = sqrt(val);
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 'p' || c == 'P') {
                double base = calc.last_result;
                double exp = string_to_double(calc.display);
                double result = power(base, exp);
                if (result != result || result > 1e15) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Power overflow");
                } else {
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 'l' || c == 'L') {
                double val = string_to_double(calc.display);
                if (val <= 0) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "ln() requires positive number");
                } else {
                    double result = logarithm(val);
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == '!') {
                int val = (int)string_to_double(calc.display);
                if (val < 0 || val > 20) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Factorial: 0-20 only");
                } else {
                    double result = factorial(val);
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 'r' || c == 'R') {
                double val = string_to_double(calc.display);
                if (val == 0) {
                    calc.error_state = 1;
                    strcpy(calc.error_message, "Cannot divide by zero");
                } else {
                    double result = 1.0 / val;
                    calc.last_result = result;
                    double_to_string(result, calc.display);
                    calc.display_pos = strlen(calc.display);
                    calc.show_result_highlight = 1;
                    calc.result_highlight_counter = 0;
                }
            }
            else if (c == 'a' || c == 'A') {
                double_to_string(PI, calc.display);
                calc.display_pos = strlen(calc.display);
                calc.show_result_highlight = 1;
                calc.result_highlight_counter = 0;
            }
            else if (c == 'b' || c == 'B') {
                double_to_string(E, calc.display);
                calc.display_pos = strlen(calc.display);
                calc.show_result_highlight = 1;
                calc.result_highlight_counter = 0;
            }
            else if (c == 'm' || c == 'M') {
                calc.memory += string_to_double(calc.display);
                calc.show_result_highlight = 1;
                calc.result_highlight_counter = 0;
            }
            else if (c == 'n' || c == 'N') {
                double_to_string(calc.memory, calc.display);
                calc.display_pos = strlen(calc.display);
                calc.show_result_highlight = 1;
                calc.result_highlight_counter = 0;
            }
            else if (c == 'x' || c == 'X') {
                calc.memory = 0;
                calc.show_result_highlight = 1;
                calc.result_highlight_counter = 0;
            }
            else if (c == 'd' || c == 'D') {
                calc.angle_mode = !calc.angle_mode;
            }
            
            calc.cursor_visible = 1;
            calc.cursor_blink_counter = 0;
        }
        
        for (volatile int i = 0; i < 100; i++) asm volatile("nop");
    }
    vga_set_color(VGA_WHITE,VGA_BLCK);
    vga_clear();
}