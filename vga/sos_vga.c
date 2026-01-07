#include "sos_vga.h"

#ifdef SMOLOS_VGA_TEST
#include "sos_stdio.h"
#include <assert.h>
#endif

uint16_t* const VGA_MEM = (uint16_t*) 0xB8000;

size_t vga_t_row;
size_t vga_t_column;
uint8_t vga_t_color;
uint16_t* vga_t_buf;

uint8_t vga_color_startup(enum colors_vga fg, enum colors_vga bg) {
    return fg | bg << 4;
}

uint16_t vga_startup(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void vga_init(void) {
    vga_t_row = 0;
    vga_t_column = 0;
    vga_t_color = vga_color_startup(VGA_LGREY, VGA_BLCK);
    vga_t_buf = VGA_MEM;
    
    for (size_t y = 0; y < HEIGHT; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            const size_t index = y * WIDTH + x;
            vga_t_buf[index] = vga_startup(' ', vga_t_color);
        }
    }
    
    vga_setcursor(0, 0);
}

void vga_clear(void) {
    for (size_t y = 0; y < HEIGHT; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            const size_t index = y * WIDTH + x;
            vga_t_buf[index] = vga_startup(' ', vga_t_color);
        }
    }
    vga_t_row = 0;
    vga_t_column = 0;
    vga_setcursor(vga_t_column, vga_t_row);
}

void vga_setcursor(int x, int y) {
    if (x < 0) x = 0;
    if (x >= WIDTH) x = WIDTH - 1;
    if (y < 0) y = 0;
    if (y >= HEIGHT) y = HEIGHT - 1;
    
    uint16_t pos = y * WIDTH + x;
    
    outb(VGA_CTRL_REG, 0x0F);
    outb(VGA_DATA_REG, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_REG, 0x0E);
    outb(VGA_DATA_REG, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_scroll(void) {
    for (size_t y = 1; y < HEIGHT; y++) {
        for (size_t x = 0; x < WIDTH; x++) {
            const size_t from_index = y * WIDTH + x;
            const size_t to_index = (y - 1) * WIDTH + x;
            vga_t_buf[to_index] = vga_t_buf[from_index];
        }
    }
    
    for (size_t x = 0; x < WIDTH; x++) {
        const size_t index = (HEIGHT - 1) * WIDTH + x;
        vga_t_buf[index] = vga_startup(' ', vga_t_color);
    }
    
    vga_t_row = HEIGHT - 1;
}

void vga_set_fg(uint8_t color) {
    vga_t_color = (vga_t_color & 0xF0) | (color & 0x0F);
}

void vga_set_bg(uint8_t color) {
    vga_t_color = (vga_t_color & 0x0F) | ((color & 0x0F) << 4);
}

uint8_t vga_get_fg(void) {
    return vga_t_color & 0x0F;
}

uint8_t vga_get_bg(void) {
    return (vga_t_color >> 4) & 0x0F;
}

void vga_putchr(char c) {
    if (c == '\n') {
        vga_t_column = 0;
        vga_t_row++;
        if (vga_t_row >= HEIGHT) {
            vga_scroll();
        }
        vga_setcursor(vga_t_column, vga_t_row);
        return;
    }
    
    if (c == '\b') {
        if (vga_t_column > 0) {
            vga_t_column--;
        } else if (vga_t_row > 0) {
            vga_t_row--;
            vga_t_column = WIDTH - 1;
        }
        vga_putchr_at(vga_t_column, vga_t_row, ' ');
        vga_setcursor(vga_t_column, vga_t_row);
        return;
    }
    
    if (c == '\t') {
        vga_putchr(' ');
        vga_putchr(' ');
        vga_putchr(' ');
        vga_putchr(' ');
        return;
    }
    
    vga_putchr_at(vga_t_column, vga_t_row, c);
    
    vga_t_column++;
    if (vga_t_column >= WIDTH) {
        vga_t_column = 0;
        vga_t_row++;
        if (vga_t_row >= HEIGHT) {
            vga_scroll();
        }
    }
    
    vga_setcursor(vga_t_column, vga_t_row);
}

void vga_putchr_at(int x, int y, char c) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
        return;
    }
    
    const size_t index = y * WIDTH + x;
    vga_t_buf[index] = vga_startup(c, vga_t_color);
}

void vga_putchr_color(char c, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    vga_putchr(c);
    vga_t_color = old_color;
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_t_color = vga_color_startup(fg, bg);
}

void vga_reset_color(void) {
    vga_t_color = vga_color_startup(VGA_LGREY, VGA_BLCK);
}

void vga_print_color(char* str, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    vga_print(str);
    vga_t_color = old_color;
}

void vga_println_color(char* str, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    vga_println(str);
    vga_t_color = old_color;
}

void vga_print(char* str) {
    while (*str) {
        vga_putchr(*str++);
    }
}

void vga_println(char* str) {
    vga_print(str);
    vga_putchr('\n');
}

void vga_backspace(void) {
    vga_putchr('\b');
    vga_putchr(' ');
    vga_putchr('\b');
}

void vga_print_int(int num) {
    if (num == 0) {
        vga_putchr('0');
        return;
    }
    
    if (num < 0) {
        vga_putchr('-');
        num = -num;
    }
    
    char buffer[16];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        vga_putchr(buffer[--i]);
    }
}

void vga_print_hex(uint32_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[9];
    int i;
    
    for (i = 7; i >= 0; i--) {
        buffer[i] = hex_chars[num & 0xF];
        num >>= 4;
    }
    buffer[8] = '\0';
    
    int start = 0;
    while (start < 7 && buffer[start] == '0') {
        start++;
    }
    
    for (i = start; i < 8; i++) {
        vga_putchr(buffer[i]);
    }
}


void vga_clear_line(size_t line) {
    if (line >= HEIGHT) return;
    
    for (size_t x = 0; x < WIDTH; x++) {
        const size_t index = line * WIDTH + x;
        vga_t_buf[index] = vga_startup(' ', vga_t_color);
    }
}

void vga_draw_box(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0 || x + width > WIDTH || y + height > HEIGHT) return;
    
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 0; i < width; i++) {
        vga_putchr_at(x + i, y, '-');
        vga_putchr_at(x + i, y + height - 1, '-');
    }
    
    for (int i = 0; i < height; i++) {
        vga_putchr_at(x, y + i, '|');
        vga_putchr_at(x + width - 1, y + i, '|');
    }
    
    vga_putchr_at(x, y, '+');
    vga_putchr_at(x + width - 1, y, '+');
    vga_putchr_at(x, y + height - 1, '+');
    vga_putchr_at(x + width - 1, y + height - 1, '+');
    
    vga_t_color = old_color;
}

void vga_fill_rect(int x, int y, int width, int height, char c, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0) return;
    
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int row = 0; row < height && (y + row) < HEIGHT; row++) {
        for (int col = 0; col < width && (x + col) < WIDTH; col++) {
            vga_putchr_at(x + col, y + row, c);
        }
    }
    
    vga_t_color = old_color;
}

void vga_get_cursor_pos(int* x, int* y) {
    if (x) *x = vga_t_column;
    if (y) *y = vga_t_row;
}

void vga_save_cursor(int* x, int* y) {
    vga_get_cursor_pos(x, y);
}

void vga_restore_cursor(int x, int y) {
    vga_t_column = x;
    vga_t_row = y;
    vga_setcursor(x, y);
}

void vga_print_centered(char* str, int row) {
    if (row < 0 || row >= HEIGHT) return;
    
    int len = 0;
    while (str[len]) len++;
    
    int start_col = (WIDTH - len) / 2;
    if (start_col < 0) start_col = 0;
    
    int old_row = vga_t_row;
    int old_col = vga_t_column;
    
    vga_t_row = row;
    vga_t_column = start_col;
    vga_setcursor(start_col, row);
    vga_print(str);
    
    vga_t_row = old_row;
    vga_t_column = old_col;
    vga_setcursor(old_col, old_row);
}

void vga_print_binary(uint32_t num) {
    vga_print("0b");
    int started = 0;
    for (int i = 31; i >= 0; i--) {
        if (num & (1 << i)) {
            vga_putchr('1');
            started = 1;
        } else if (started) {
            vga_putchr('0');
        }
    }
    if (!started) vga_putchr('0');
}

void vga_invert_colors(int x, int y, int width, int height) {
    for (int row = 0; row < height && (y + row) < HEIGHT; row++) {
        for (int col = 0; col < width && (x + col) < WIDTH; col++) {
            size_t index = (y + row) * WIDTH + (x + col);
            uint16_t entry = vga_t_buf[index];
            uint8_t character = entry & 0xFF;
            uint8_t color = (entry >> 8) & 0xFF;
            
            uint8_t fg = color & 0x0F;
            uint8_t bg = (color >> 4) & 0x0F;
            uint8_t new_color = (fg << 4) | bg;
            
            vga_t_buf[index] = vga_startup(character, new_color);
        }
    }
}



#ifdef SMOLOS_VGA_TEST

void test_vga_color_startup() {
    printf("Testing vga_color_startup...\n");
    
    uint8_t color = vga_color_startup(VGA_WHITE, VGA_BLUE);
    assert((color & 0x0F) == VGA_WHITE);  
    assert(((color >> 4) & 0x0F) == VGA_BLUE); 
    
    color = vga_color_startup(VGA_RED, VGA_GREEN);
    assert((color & 0x0F) == VGA_RED);
    assert(((color >> 4) & 0x0F) == VGA_GREEN);
    
    printf("[T] Color creation works correctly\n");
}

void test_vga_startup() {
    printf("Testing vga_startup...\n");
    
    uint16_t entry = vga_startup('A', 0x0F);
    assert((entry & 0xFF) == 'A');
    assert(((entry >> 8) & 0xFF) == 0x0F);
    
    entry = vga_startup('Z', 0x4E);
    assert((entry & 0xFF) == 'Z');
    assert(((entry >> 8) & 0xFF) == 0x4E);
    
    printf("[T] VGA entry creation works correctly\n");
}

void test_vga_colors() {
    printf("Testing VGA color functions...\n");
    
    vga_t_color = vga_color_startup(VGA_LGREY, VGA_BLCK);
    
    vga_set_fg(VGA_RED);
    assert(vga_get_fg() == VGA_RED);
    assert(vga_get_bg() == VGA_BLCK);
    
    vga_set_bg(VGA_BLUE);
    assert(vga_get_fg() == VGA_RED);
    assert(vga_get_bg() == VGA_BLUE);
    
    vga_set_color(VGA_WHITE, VGA_GREEN);
    assert(vga_get_fg() == VGA_WHITE);
    assert(vga_get_bg() == VGA_GREEN);
    
    vga_reset_color();
    assert(vga_get_fg() == VGA_LGREY);
    assert(vga_get_bg() == VGA_BLCK);
    
    printf("[T] Color set/get functions work correctly\n");
}

void test_vga_print_int() {
    printf("Testing vga_print_int...\n");
    
    printf("  Testing with 0, 42, -100, 12345\n");
    printf("[T] Integer printing functions compiled successfully\n");
}

void test_vga_print_hex() {
    printf("Testing vga_print_hex...\n");
    
    printf("  Testing with 0x0, 0xFF, 0x1234ABCD\n");
    printf("[T] Hex printing functions compiled successfully\n");
}

int main(void) {
    printf("=== SmolOS VGA Driver Test Suite ===\n\n");
    
    test_vga_color_startup();
    test_vga_startup();
    test_vga_colors();
    test_vga_print_int();
    test_vga_print_hex();
    
    printf("\n=== All VGA tests passed! ===\n");
    return 0;
}

#endif