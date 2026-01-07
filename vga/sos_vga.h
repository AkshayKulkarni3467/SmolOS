#ifndef INCLUDE_SMOLOS_VGA_H
#define INCLUDE_SMOLOS_VGA_H

#include "sos_stdint.h"
#include "sos_stddef.h"

#define WIDTH 80
#define HEIGHT 25

#define VGA_CTRL_REG 0x3D4
#define VGA_DATA_REG 0x3D5

enum colors_vga {
    VGA_BLCK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BRWN = 6,
    VGA_LGREY = 7,
    VGA_DGREY = 8,
    VGA_LBLUE = 9,
    VGA_LGREEN = 10,
    VGA_LCYAN = 11,
    VGA_LRED = 12,
    VGA_LMAGENTA = 13,
    VGA_LBRWN = 14,
    VGA_WHITE = 15
};


void vga_init(void);
void vga_clear(void);

uint8_t vga_color_startup(enum colors_vga fg, enum colors_vga bg);
uint16_t vga_startup(unsigned char uc, uint8_t color);
void outb(uint16_t port, uint8_t value);


void vga_setcursor(int x, int y);
void vga_scroll(void);

void vga_set_fg(uint8_t color);
void vga_set_bg(uint8_t color);
uint8_t vga_get_fg(void);
uint8_t vga_get_bg(void);

void vga_putchr(char c);
void vga_putchr_at(int x, int y, char c);
void vga_putchr_color(char c, uint8_t fg, uint8_t bg);

void vga_set_color(uint8_t fg, uint8_t bg);
void vga_reset_color(void);
void vga_print_color(char* s, uint8_t fg, uint8_t bg);
void vga_println_color(char* s, uint8_t fg, uint8_t bg);


void vga_print(char* s);
void vga_println(char* s);

void vga_backspace(void);
void vga_print_int(int num);
void vga_print_hex(uint32_t num);


#endif // INCLUDE_SMOLOS_VGA_H
