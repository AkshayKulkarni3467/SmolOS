#ifndef INCLUDE_SMOLOS_VGA_H
#define INCLUDE_SMOLOS_VGA_H

#include "sos_stdint.h"
#include "sos_stddef.h"

#define WIDTH 80
#define HEIGHT 25

#define VGA_CTRL_REG 0x3D4
#define VGA_DATA_REG 0x3D5

static const uint16_t* VGA_MEM = (uint16_t*) 0xB8000;

static size_t vga_t_row;
static size_t vga_t_column;
static uint8_t vga_t_color;
static uint16_t* vga_t_buf;

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
    VGA_WHITE = 15,
    VGA_YELLOW = 14
};


void vga_init(void);
void vga_clear(void);

uint8_t vga_color_startup(enum colors_vga fg, enum colors_vga bg);
uint16_t vga_startup(unsigned char uc, uint8_t color);


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

void vga_clear_line(size_t line);
void vga_draw_box(int x, int y, int width, int height, uint8_t fg, uint8_t bg);
void vga_fill_rect(int x, int y, int width, int height, char c, uint8_t fg, uint8_t bg);
void vga_get_cursor_pos(int* x, int* y);
void vga_save_cursor(int* x, int* y);
void vga_restore_cursor(int x, int y);
void vga_print_centered(char* str, int row);
void vga_print_binary(uint32_t num);
void vga_invert_colors(int x, int y, int width, int height);




#endif // INCLUDE_SMOLOS_VGA_H
