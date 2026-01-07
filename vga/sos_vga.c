#include "sos_vga.h"

#ifdef SMOLOS_VGA_TEST

#include "sos_stdio.h"

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

#ifdef SMOLOS_VGA_TEST

int main(void){
    printf("Hello from VGA!\n");
    return 0;
}

#endif