#ifndef INCLUDE_SMOLOS_VGRAPHICS_H
#define INCLUDE_SMOLOS_VGRAPHICS_H

#include "stdint.h"

void vga_draw_line_horizontal(int x, int y, int length, char c, uint8_t fg, uint8_t bg);
void vga_draw_line_vertical(int x, int y, int length, char c, uint8_t fg, uint8_t bg);

void vga_draw_box(int x, int y, int width, int height, uint8_t fg, uint8_t bg);
void vga_draw_box_single(int x, int y, int width, int height, uint8_t fg, uint8_t bg);
void vga_draw_box_double(int x, int y, int width, int height, uint8_t fg, uint8_t bg);
void vga_draw_shadow_box(int x, int y, int width, int height, uint8_t fg, uint8_t bg);
void vga_draw_fancy_border(int x, int y, int width, int height, uint8_t fg, uint8_t bg);

void vga_fill_rect(int x, int y, int width, int height, char c, uint8_t fg, uint8_t bg);

void vga_draw_window(int x, int y, int width, int height, char* title, uint8_t fg, uint8_t bg);
void vga_draw_progress_bar(int x, int y, int width, int progress, uint8_t fg_full, uint8_t fg_empty, uint8_t bg);
void vga_draw_spinner(int x, int y, int frame, uint8_t fg, uint8_t bg);
void vga_draw_menu_item(int x, int y, int width, char* text, int selected, uint8_t fg, uint8_t bg);
void vga_draw_table_border(int x, int y, int width, int height, uint8_t fg, uint8_t bg);
void vga_draw_separator(int x, int y, int width, uint8_t fg, uint8_t bg);

void vga_checkerboard(int x, int y, int width, int height, uint8_t fg1, uint8_t bg1, uint8_t fg2, uint8_t bg2);
void vga_gradient_horizontal(int x, int y, int width, int height);
void vga_rainbow_text(char* text, int x, int y);
void vga_flash_screen(uint8_t fg, uint8_t bg, int duration);
void vga_animate_text(char* text, int x, int y, int delay, uint8_t fg, uint8_t bg);

void vga_draw_smiley(int x, int y, uint8_t fg, uint8_t bg);
void vga_draw_heart(int x, int y, uint8_t fg, uint8_t bg);
void vga_draw_arrow_right(int x, int y, int length, uint8_t fg, uint8_t bg);
void vga_draw_star(int x, int y, uint8_t fg, uint8_t bg);

void vga_draw_battery(int x, int y, int level, uint8_t fg, uint8_t bg);
void vga_draw_loading_dots(int x, int y, int count, uint8_t fg, uint8_t bg);
void vga_draw_status_indicator(int x, int y, int active, char* label, uint8_t fg, uint8_t bg);

void vga_print_shadowed(char* text, int x, int y, uint8_t fg, uint8_t bg);
void vga_print_boxed(char* text, int x, int y, uint8_t fg, uint8_t bg);
void vga_print_highlighted(char* text, int x, int y, uint8_t fg, uint8_t bg);

#endif //INCLUDE_SMOLOS_VGRAPHICS_H