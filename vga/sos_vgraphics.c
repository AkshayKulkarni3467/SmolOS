#include "sos_vga.h"

void vga_draw_line_horizontal(int x, int y, int length, char c, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 0; i < length && (x + i) < WIDTH; i++) {
        vga_putchr_at(x + i, y, c);
    }
    
    vga_t_color = old_color;
}

void vga_draw_line_vertical(int x, int y, int length, char c, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 0; i < length && (y + i) < HEIGHT; i++) {
        vga_putchr_at(x, y + i, c);
    }
    
    vga_t_color = old_color;
}

void vga_draw_box_double(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0 || x + width > WIDTH || y + height > HEIGHT) return;
    
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    

    for (int i = 1; i < width - 1; i++) {
        vga_putchr_at(x + i, y, 0xCD);  
        vga_putchr_at(x + i, y + height - 1, 0xCD);
    }
    
    for (int i = 1; i < height - 1; i++) {
        vga_putchr_at(x, y + i, 0xBA);  
        vga_putchr_at(x + width - 1, y + i, 0xBA);
    }
    
    vga_putchr_at(x, y, 0xC9);                          
    vga_putchr_at(x + width - 1, y, 0xBB);              
    vga_putchr_at(x, y + height - 1, 0xC8);             
    vga_putchr_at(x + width - 1, y + height - 1, 0xBC); 
    
    vga_t_color = old_color;
}

void vga_draw_box_single(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {
    if (x < 0 || y < 0 || x + width > WIDTH || y + height > HEIGHT) return;
    
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 1; i < width - 1; i++) {
        vga_putchr_at(x + i, y, 0xC4); 
        vga_putchr_at(x + i, y + height - 1, 0xC4);
    }
    
    for (int i = 1; i < height - 1; i++) {
        vga_putchr_at(x, y + i, 0xB3);  
        vga_putchr_at(x + width - 1, y + i, 0xB3);
    }
    
    vga_putchr_at(x, y, 0xDA);                          
    vga_putchr_at(x + width - 1, y, 0xBF);              
    vga_putchr_at(x, y + height - 1, 0xC0);             
    vga_putchr_at(x + width - 1, y + height - 1, 0xD9); 
    
    vga_t_color = old_color;
}

void vga_draw_shadow_box(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {

    vga_draw_box_double(x, y, width, height, fg, bg);
    
    uint8_t shadow_color = vga_color_startup(VGA_DGREY, VGA_BLCK);
    
    for (int i = 1; i <= height && (y + i) < HEIGHT && (x + width) < WIDTH; i++) {
        size_t index = (y + i) * WIDTH + (x + width);
        uint16_t entry = vga_t_buf[index];
        vga_t_buf[index] = (entry & 0xFF) | ((uint16_t)shadow_color << 8);
    }
    
    for (int i = 1; i <= width && (x + i) < WIDTH && (y + height) < HEIGHT; i++) {
        size_t index = (y + height) * WIDTH + (x + i);
        uint16_t entry = vga_t_buf[index];
        vga_t_buf[index] = (entry & 0xFF) | ((uint16_t)shadow_color << 8);
    }
}

void vga_draw_window(int x, int y, int width, int height, char* title, uint8_t fg, uint8_t bg) {

    vga_draw_box_double(x, y, width, height, fg, bg);
    
    uint8_t title_color = vga_color_startup(VGA_WHITE, VGA_BLUE);
    for (int i = 1; i < width - 1; i++) {
        size_t index = (y + 1) * WIDTH + (x + i);
        vga_t_buf[index] = vga_startup(' ', title_color);
    }
    
    int title_len = 0;
    while (title[title_len]) title_len++;
    
    int title_x = x + (width - title_len) / 2;
    uint8_t old_color = vga_t_color;
    vga_t_color = title_color;
    
    for (int i = 0; i < title_len && (title_x + i) < (x + width - 1); i++) {
        vga_putchr_at(title_x + i, y + 1, title[i]);
    }
    
    vga_t_color = old_color;
    
    for (int i = 1; i < width - 1; i++) {
        vga_putchr_at(x + i, y + 2, 0xC4);
    }
    vga_putchr_at(x, y + 2, 0xC3);         
    vga_putchr_at(x + width - 1, y + 2, 0xB4); 
}

void vga_draw_progress_bar(int x, int y, int width, int progress, uint8_t fg_full, uint8_t fg_empty, uint8_t bg) {
    if (progress < 0) progress = 0;
    if (progress > 100) progress = 100;
    
    int filled = (width * progress) / 100;
    
    uint8_t old_color = vga_t_color;
    
    vga_set_color(fg_full, bg);
    for (int i = 0; i < filled; i++) {
        vga_putchr_at(x + i, y, 0xDB); 
    }
    
    vga_set_color(fg_empty, bg);
    for (int i = filled; i < width; i++) {
        vga_putchr_at(x + i, y, 0xB0); 
    }
    
    vga_t_color = old_color;
}

void vga_draw_spinner(int x, int y, int frame, uint8_t fg, uint8_t bg) {
    char spinners[] = {'|', '/', '-', '\\'};
    vga_putchr_color(spinners[frame % 4], fg, bg);
}

void vga_draw_menu_item(int x, int y, int width, char* text, int selected, uint8_t fg, uint8_t bg) {
    uint8_t item_fg = selected ? VGA_BLCK : fg;
    uint8_t item_bg = selected ? VGA_CYAN : bg;
    
    uint8_t old_color = vga_t_color;
    vga_set_color(item_fg, item_bg);
    
    for (int i = 0; i < width; i++) {
        vga_putchr_at(x + i, y, ' ');
    }
    
    if (selected) {
        vga_putchr_at(x + 1, y, '>');
    }
    
    int text_x = selected ? x + 3 : x + 2;
    int i = 0;
    while (text[i] && (text_x + i) < (x + width - 1)) {
        vga_putchr_at(text_x + i, y, text[i]);
        i++;
    }
    
    vga_t_color = old_color;
}

void vga_draw_table_border(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {
    vga_draw_box_single(x, y, width, height, fg, bg);
}

void vga_draw_separator(int x, int y, int width, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 0; i < width; i++) {
        vga_putchr_at(x + i, y, 0xC4); 
    }
    
    vga_t_color = old_color;
}


void vga_checkerboard(int x, int y, int width, int height, uint8_t fg1, uint8_t bg1, uint8_t fg2, uint8_t bg2) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int is_even = ((row + col) % 2) == 0;
            uint8_t fg = is_even ? fg1 : fg2;
            uint8_t bg = is_even ? bg1 : bg2;
            vga_putchr_color(' ', fg, bg);
        }
    }
}

void vga_gradient_horizontal(int x, int y, int width, int height) {
    uint8_t colors[] = {VGA_BLCK, VGA_BLUE, VGA_CYAN, VGA_GREEN, VGA_YELLOW, VGA_RED, VGA_MAGENTA, VGA_WHITE};
    int num_colors = 8;
    
    uint8_t old_color = vga_t_color;
    
    for (int col = 0; col < width && (x + col) < WIDTH; col++) {
        int color_idx = (col * num_colors) / width;
        if (color_idx >= num_colors) color_idx = num_colors - 1;
        
        vga_set_color(VGA_BLCK, colors[color_idx]);
        
        for (int row = 0; row < height && (y + row) < HEIGHT; row++) {
            vga_putchr_at(x + col, y + row, ' ');
        }
    }
    
    vga_t_color = old_color;
}

void vga_rainbow_text(char* text, int x, int y) {
    uint8_t colors[] = {VGA_RED, VGA_LRED, VGA_YELLOW, VGA_GREEN, VGA_CYAN, VGA_BLUE, VGA_MAGENTA};
    int num_colors = 7;
    
    uint8_t old_color = vga_t_color;
    
    int i = 0;
    while (text[i]) {
        vga_set_color(colors[i % num_colors], VGA_BLCK);
        vga_putchr_at(x + i, y, text[i]);
        i++;
    }
    
    vga_t_color = old_color;
}

void vga_flash_screen(uint8_t fg, uint8_t bg, int duration) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    uint16_t saved[WIDTH * HEIGHT];
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        saved[i] = vga_t_buf[i];
    }
    
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        vga_t_buf[i] = vga_startup(' ', vga_t_color);
    }
    
    for (volatile int i = 0; i < duration * 100000; i++);
    
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        vga_t_buf[i] = saved[i];
    }
    
    vga_t_color = old_color;
}

void vga_animate_text(char* text, int x, int y, int delay, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    int i = 0;
    while (text[i]) {
        vga_putchr_at(x + i, y, text[i]);
        
        for (volatile int d = 0; d < delay * 1000000; d++);
        
        i++;
    }
    
    vga_t_color = old_color;
}


void vga_draw_smiley(int x, int y, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    vga_putchr_at(x + 1, y, 'o');    
    vga_putchr_at(x + 3, y, 'o');    
    vga_putchr_at(x, y + 1, '\\');   
    vga_putchr_at(x + 1, y + 2, '\\'); 
    vga_putchr_at(x + 2, y + 2, '_');
    vga_putchr_at(x + 3, y + 2, '/');
    vga_putchr_at(x + 4, y + 1, '/'); 
    
    vga_t_color = old_color;
}

void vga_draw_heart(int x, int y, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    vga_putchr_at(x, y, ' ');
    vga_putchr_at(x + 1, y, 0x03);  
    vga_putchr_at(x + 2, y, ' ');
    
    vga_t_color = old_color;
}

void vga_draw_arrow_right(int x, int y, int length, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 0; i < length - 1; i++) {
        vga_putchr_at(x + i, y, '-');
    }
    vga_putchr_at(x + length - 1, y, '>');
    
    vga_t_color = old_color;
}

void vga_draw_star(int x, int y, uint8_t fg, uint8_t bg) {
    vga_putchr_color('*', fg, bg);
}


void vga_draw_battery(int x, int y, int level, uint8_t fg, uint8_t bg) {
    if (level < 0) level = 0;
    if (level > 100) level = 100;
    
    uint8_t old_color = vga_t_color;
    
    vga_set_color(fg, bg);
    vga_putchr_at(x, y, '[');
    vga_putchr_at(x + 11, y, ']');
    vga_putchr_at(x + 12, y, '+');
    
    int segments = (level * 10) / 100;
    
    uint8_t fill_color = level > 20 ? VGA_GREEN : VGA_RED;
    vga_set_color(fill_color, bg);
    
    for (int i = 0; i < 10; i++) {
        char c = (i < segments) ? 0xDB : ' '; 
        vga_putchr_at(x + 1 + i, y, c);
    }
    
    vga_t_color = old_color;
}

void vga_draw_loading_dots(int x, int y, int count, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    int dots = count % 4;
    vga_print("Loading");
    
    for (int i = 0; i < 3; i++) {
        vga_putchr(i < dots ? '.' : ' ');
    }
    
    vga_t_color = old_color;
}

void vga_draw_status_indicator(int x, int y, int active, char* label, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    
    uint8_t indicator_color = active ? VGA_GREEN : VGA_RED;
    vga_set_color(indicator_color, bg);
    vga_putchr_at(x, y, 0x07); 
    
    vga_set_color(fg, bg);
    int i = 0;
    while (label[i]) {
        vga_putchr_at(x + 2 + i, y, label[i]);
        i++;
    }
    
    vga_t_color = old_color;
}


void vga_draw_fancy_border(int x, int y, int width, int height, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    vga_putchr_at(x, y, 0xC9);  
    for (int i = 1; i < width - 1; i++) {
        vga_putchr_at(x + i, y, (i % 2) ? 0xCD : 0xCE);  
    }
    vga_putchr_at(x + width - 1, y, 0xBB);  
    
    for (int i = 1; i < height - 1; i++) {
        vga_putchr_at(x, y + i, 0xBA);  
        vga_putchr_at(x + width - 1, y + i, 0xBA);
    }
    
    vga_putchr_at(x, y + height - 1, 0xC8);  
    for (int i = 1; i < width - 1; i++) {
        vga_putchr_at(x + i, y + height - 1, 0xCD);
    }
    vga_putchr_at(x + width - 1, y + height - 1, 0xBC);  
    
    vga_t_color = old_color;
}


void vga_print_shadowed(char* text, int x, int y, uint8_t fg, uint8_t bg) {
    uint8_t old_color = vga_t_color;
    vga_set_color(VGA_DGREY, bg);
    
    int i = 0;
    while (text[i]) {
        vga_putchr_at(x + 1 + i, y + 1, text[i]);
        i++;
    }
    
    vga_set_color(fg, bg);
    i = 0;
    while (text[i]) {
        vga_putchr_at(x + i, y, text[i]);
        i++;
    }
    
    vga_t_color = old_color;
}

void vga_print_boxed(char* text, int x, int y, uint8_t fg, uint8_t bg) {
    int len = 0;
    while (text[len]) len++;
    
    int width = len + 4;  
    int height = 3;
    
    vga_draw_box_single(x, y, width, height, fg, bg);
    
    uint8_t old_color = vga_t_color;
    vga_set_color(fg, bg);
    
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + 2 + i, y + 1, text[i]);
    }
    
    vga_t_color = old_color;
}

void vga_print_highlighted(char* text, int x, int y, uint8_t fg, uint8_t bg) {
    int len = 0;
    while (text[len]) len++;
    
    uint8_t old_color = vga_t_color;
    
    vga_set_color(fg, bg);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + i, y, ' ');
    }
    
    vga_set_color(bg, fg);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + i, y, text[i]);
    }
    
    vga_t_color = old_color;
}