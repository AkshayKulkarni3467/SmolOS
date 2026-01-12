#include "sos_artgallery.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_memory.h"
#include "sos_pit.h"

static uint32_t rand_seed = 12345;

void art_init_random(void) {
    rand_seed = 12345;
}

uint32_t art_rand(void) {
    rand_seed = (rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return rand_seed;
}

uint8_t art_get_color(int value, int max) {
    if (max == 0) return VGA_BLCK;
    int ratio = (value * 15) / max;
    if (ratio < 0) ratio = 0;
    if (ratio > 15) ratio = 15;
    
    static const uint8_t gradient[] = {
        VGA_BLCK, VGA_BLUE, VGA_BLUE, VGA_LBLUE,
        VGA_CYAN, VGA_LCYAN, VGA_GREEN, VGA_LGREEN,
        VGA_YELLOW, VGA_LRED, VGA_RED, VGA_MAGENTA,
        VGA_LMAGENTA, VGA_WHITE, VGA_WHITE, VGA_WHITE
    };
    return gradient[ratio];
}

void art_fade_screen(void) {
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            size_t index = y * 80 + x;
            uint16_t entry = VGA_MEM[index];
            uint8_t c = entry & 0xFF;
            uint8_t color = (entry >> 8) & 0xFF;
            
            if (art_rand() % 3 == 0) {
                if (color > 0) color--;
            }
            VGA_MEM[index] = c | (color << 8);
        }
    }
}

void art_matrix_rain(void) {
    const char* chars = "01abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()";
    int drops[80];
    int speeds[80];
    uint8_t trail_length[80];
    
    for (int i = 0; i < 80; i++) {
        drops[i] = -(art_rand() % 25);
        speeds[i] = 2 + (art_rand() % 4); 
        trail_length[i] = 8 + (art_rand() % 8);
    }
    
    vga_clear();
    int running = 1;
    int frame = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        if (frame % 3 == 0) {
            for (int y = 0; y < 25; y++) {
                for (int x = 0; x < 80; x++) {
                    if (art_rand() % 15 == 0) {
                        vga_putchr_at(x, y, ' ');
                    }
                }
            }
        }
        
        for (int x = 0; x < 80; x++) {
            if (frame % speeds[x] == 0) {
                drops[x]++;
                
                for (int t = 0; t < trail_length[x]; t++) {
                    int y = drops[x] - t;
                    if (y >= 0 && y < 25) {
                        int idx = art_rand() % 66;
                        uint8_t color;
                        if (t == 0) color = VGA_WHITE;
                        else if (t < 3) color = VGA_LGREEN;
                        else if (t < 6) color = VGA_GREEN;
                        else color = VGA_DGREY;
                        
                        vga_set_color(color, VGA_BLCK);
                        vga_putchr_at(x, y, chars[idx]);
                    }
                }
                
                if (drops[x] > 30) {
                    drops[x] = -(art_rand() % 20);
                    speeds[x] = 2 + (art_rand() % 4);
                    trail_length[x] = 8 + (art_rand() % 8);
                }
            }
        }
        
        frame++;
        pit_delay_ms(100);
    }
}


void art_aurora_waves(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 80; x++) {
                int wave1 = ((x + time) % 40) * 255 / 40;
                int wave2 = ((y * 2 + time / 2) % 30) * 255 / 30;
                int wave3 = (((x + y) + time) % 50) * 255 / 50;
                
                int value = (wave1 + wave2 + wave3) / 3;
                
                uint8_t color;
                char c = 0xB0;
                
                if (value < 40) { color = VGA_BLCK; c = ' '; }
                else if (value < 80) { color = VGA_BLUE; }
                else if (value < 120) { color = VGA_LBLUE; }
                else if (value < 160) { color = VGA_CYAN; c = 0xB1; }
                else if (value < 200) { color = VGA_LCYAN; c = 0xB2; }
                else { color = VGA_WHITE; c = 0xDB; }
                
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(x, y, c);
            }
        }
        
        time++;
        pit_delay_ms(100);
    }
}


void art_fire(void) {
    typedef struct { int x, y, vx, vy, size; uint8_t heat; } Bubble;
    Bubble bubbles[15];
    
    for (int i = 0; i < 15; i++) {
        bubbles[i].x = (art_rand() % 70) + 5;
        bubbles[i].y = 20 + (art_rand() % 5);
        bubbles[i].vx = (art_rand() % 3) - 1;
        bubbles[i].vy = -(art_rand() % 3) - 1;
        bubbles[i].size = 2 + (art_rand() % 3);
        bubbles[i].heat = 100 + (art_rand() % 155);
    }
    
    vga_clear();
    int running = 1;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        if (art_rand() % 3 == 0) {
            for (int i = 0; i < 30; i++) {
                int x = art_rand() % 80;
                int y = art_rand() % 25;
                vga_putchr_at(x, y, ' ');
            }
        }
        
        for (int i = 0; i < 15; i++) {
            bubbles[i].x += bubbles[i].vx;
            bubbles[i].y += bubbles[i].vy;
            
            if (bubbles[i].x < 5 || bubbles[i].x > 75) bubbles[i].vx = -bubbles[i].vx;
            if (bubbles[i].y < 0) {
                bubbles[i].y = 24;
                bubbles[i].x = (art_rand() % 70) + 5;
                bubbles[i].heat = 100 + (art_rand() % 155);
            }
            if (bubbles[i].y > 24) bubbles[i].vy = -(art_rand() % 3) - 1;
            
            uint8_t color = bubbles[i].heat > 200 ? VGA_YELLOW :
                           bubbles[i].heat > 150 ? VGA_LRED :
                           bubbles[i].heat > 100 ? VGA_RED : VGA_MAGENTA;
            
            for (int dy = -bubbles[i].size; dy <= bubbles[i].size; dy++) {
                for (int dx = -bubbles[i].size; dx <= bubbles[i].size; dx++) {
                    int nx = bubbles[i].x + dx;
                    int ny = bubbles[i].y + dy;
                    if (nx >= 0 && nx < 80 && ny >= 0 && ny < 25) {
                        if (dx*dx + dy*dy <= bubbles[i].size * bubbles[i].size) {
                            vga_set_color(color, VGA_BLCK);
                            vga_putchr_at(nx, ny, 0xDB);
                        }
                    }
                }
            }
        }
        
        pit_delay_ms(100);
    }
}


void art_kaleidoscope(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int r = 5; r < 40; r += 5) {
            uint8_t colors[] = {VGA_RED, VGA_YELLOW, VGA_GREEN, VGA_CYAN, VGA_BLUE, VGA_MAGENTA};
            uint8_t color = colors[(r / 5 + time / 10) % 6];
            
            for (int angle = 0; angle < 360; angle += 15) {
                int a = (angle + time * 2) % 360;
                int x = 40 + (r * (a < 90 || a > 270 ? 1 : -1)) / 2;
                int y = 12 + (r * (a < 180 ? 1 : -1)) / 4;
                
                if (x >= 0 && x < 80 && y >= 0 && y < 25) {
                    vga_set_color(color, VGA_BLCK);
                    char c = (r % 10 == 0) ? '*' : (r % 10 == 5) ? '+' : '.';
                    vga_putchr_at(x, y, c);
                }
            }
        }
        
        time++;
        pit_delay_ms(100);
    }
}


void art_starfield(void) {
    typedef struct {
        int x, y, z;
        uint8_t color;
    } Star;
    
    Star stars[150];  
    
    for (int i = 0; i < 150; i++) {
        stars[i].x = (art_rand() % 160) - 80;
        stars[i].y = (art_rand() % 50) - 25;
        stars[i].z = art_rand() % 100 + 1;
        stars[i].color = VGA_WHITE;
    }
    
    vga_clear();
    int running = 1;
    int speed = 2;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int i = 0; i < 150; i++) {
            stars[i].z -= speed;
            if (stars[i].z <= 0) {
                stars[i].x = (art_rand() % 160) - 80;
                stars[i].y = (art_rand() % 50) - 25;
                stars[i].z = 100;
            }
            
            int sx = 40 + (stars[i].x * 40) / stars[i].z;
            int sy = 12 + (stars[i].y * 12) / stars[i].z;
            
            if (sx >= 0 && sx < 80 && sy >= 0 && sy < 25) {
                uint8_t color;
                char c;
                if (stars[i].z > 80) { color = VGA_DGREY; c = '.'; }
                else if (stars[i].z > 60) { color = VGA_LGREY; c = '.'; }
                else if (stars[i].z > 40) { color = VGA_WHITE; c = '*'; }
                else if (stars[i].z > 20) { color = VGA_YELLOW; c = '*'; }
                else { color = VGA_LRED; c = 0xDB; }
                
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(sx, sy, c);
            }
        }
        
        pit_delay_ms(100);
    }
}


void draw_branch(int x, int y, int len, int angle, int depth, uint8_t color) {
    if (depth == 0 || len < 1) return;
    
    int dx = (len * (angle < 90 || angle > 270 ? 1 : -1)) / 3;
    int dy = (len * (angle < 180 ? -1 : 1)) / 6;
    int nx = x + dx;
    int ny = y + dy;
    
    int steps = len;
    for (int i = 0; i <= steps; i++) {
        int px = x + (dx * i) / steps;
        int py = y + (dy * i) / steps;
        if (px >= 0 && px < 80 && py >= 0 && py < 25) {
            vga_set_color(color, VGA_BLCK);
            vga_putchr_at(px, py, '|');
        }
    }
    
    uint8_t new_color = (color == VGA_BRWN) ? VGA_GREEN : 
                       (color == VGA_GREEN) ? VGA_LGREEN : VGA_YELLOW;
    draw_branch(nx, ny, len * 2 / 3, (angle + 30) % 360, depth - 1, new_color);
    draw_branch(nx, ny, len * 2 / 3, (angle - 30 + 360) % 360, depth - 1, new_color);
}

void art_fractal(void) {
    vga_clear();
    

    
    draw_branch(20, 24, 15, 90, 6, VGA_BRWN);
    draw_branch(45, 24, 60, 90, 12, VGA_BRWN);  
    draw_branch(60, 24, 32, 90, 6, VGA_BRWN);  
    wait_for_char();
}


void art_ripple(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    int cx = 40, cy = 12;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int r = 0; r < 50; r++) {
            int actual_r = (r + time) % 50;
            uint8_t color;
            char c;
            
            if (actual_r < 10) { color = VGA_BLUE; c = 0xB0; }
            else if (actual_r < 20) { color = VGA_CYAN; c = 0xB1; }
            else if (actual_r < 30) { color = VGA_LCYAN; c = 0xB2; }
            else if (actual_r < 40) { color = VGA_WHITE; c = 0xDB; }
            else { color = VGA_YELLOW; c = 0xDB; }
            
            for (int angle = 0; angle < 360; angle += 5) {
                int x = cx + (actual_r * (angle < 90 || angle > 270 ? 1 : -1)) / 2;
                int y = cy + (actual_r * (angle < 180 ? 1 : -1)) / 4;
                
                if (x >= 0 && x < 80 && y >= 0 && y < 25) {
                    vga_set_color(color, VGA_BLCK);
                    vga_putchr_at(x, y, c);
                }
            }
        }
        
        time++;
        for (volatile int i = 0; i < 50000; i++);
    }
}


void art_metaballs(void) {
    typedef struct { int x, y, vx, vy; uint8_t col; } Ball;
    int num_ball = 6;
    Ball balls[num_ball];
    
    uint8_t colors[] = {VGA_RED, VGA_YELLOW, VGA_GREEN, VGA_CYAN, VGA_BLUE, VGA_MAGENTA};
    
    for (int i = 0; i < num_ball; i++) {
        balls[i].x = art_rand() % 80;
        balls[i].y = art_rand() % 25;
        balls[i].vx = (art_rand() % 3) - 1;
        balls[i].vy = (art_rand() % 3) - 1;
        balls[i].col = colors[i];
    }
    
    vga_clear();
    int running = 1;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        for (int i = 0; i < num_ball; i++) {
            balls[i].x += balls[i].vx;
            balls[i].y += balls[i].vy;
            
            if (balls[i].x <= 0 || balls[i].x >= 79) balls[i].vx = -balls[i].vx;
            if (balls[i].y <= 0 || balls[i].y >= 24) balls[i].vy = -balls[i].vy;
        }
        
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 80; x++) {
                int sum = 0;
                int closest = 0;
                int min_dist = 999999;
                
                for (int i = 0; i < num_ball; i++) {
                    int dx = x - balls[i].x;
                    int dy = y - balls[i].y;
                    int dist = dx*dx + dy*dy + 1;
                    sum += 150 / dist;
                    
                    if (dist < min_dist) {
                        min_dist = dist;
                        closest = i;
                    }
                }
                
                uint8_t color;
                char c;
                if (sum < 15) { color = VGA_BLCK; c = ' '; }
                else if (sum < 30) { color = balls[closest].col; c = 0xB0; }
                else if (sum < 50) { color = balls[closest].col; c = 0xB1; }
                else if (sum < 70) { color = balls[closest].col; c = 0xB2; }
                else { color = balls[closest].col; c = 0xDB; }
                
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(x, y, c);
            }
        }
        
        pit_delay_ms(100);
    }
}


void art_particles(void) {
    typedef struct { int x, y, vx, vy; uint8_t life; uint8_t type; } Particle;
    Particle particles[250];
    
    for (int i = 0; i < 250; i++) {
        particles[i].x = 400;
        particles[i].y = 120;
        particles[i].vx = (art_rand() % 50) - 25;
        particles[i].vy = (art_rand() % 50) - 30;
        particles[i].life = art_rand() % 120 + 60;
        particles[i].type = art_rand() % 3;
    }
    
    vga_clear();
    int running = 1;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        if (art_rand() % 2 == 0) {
            for (int i = 0; i < 120; i++) {
                int x = art_rand() % 80;
                int y = art_rand() % 25;
                vga_putchr_at(x, y, ' ');
            }
        }
        
        for (int i = 0; i < 250; i++) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy += 3; 
            particles[i].life--;
            
            if (particles[i].life <= 0 || particles[i].y > 240) {
                particles[i].x = 400;
                particles[i].y = 120;
                particles[i].vx = (art_rand() % 50) - 25;
                particles[i].vy = (art_rand() % 50) - 30;
                particles[i].life = art_rand() % 120 + 60;
                particles[i].type = art_rand() % 3;
            }
            
            int sx = particles[i].x / 10;
            int sy = particles[i].y / 10;
            
            if (sx >= 0 && sx < 80 && sy >= 0 && sy < 25) {
                uint8_t color;
                char c;
                
                if (particles[i].life > 100) { 
                    color = VGA_WHITE; c = '*'; 
                }
                else if (particles[i].life > 70) { 
                    color = VGA_YELLOW; c = particles[i].type == 0 ? '*' : '+';
                }
                else if (particles[i].life > 40) { 
                    color = VGA_LRED; c = particles[i].type == 1 ? '+' : '.';
                }
                else { 
                    color = VGA_RED; c = '.';
                }
                
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(sx, sy, c);
            }
        }
        
        pit_delay_ms(100);
    }
}

void art_dna_helix(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int x = 0; x < 80; x++) {
            int angle1 = (x * 9 + time * 2) % 360;
            int angle2 = (angle1 + 180) % 360;
            
            int y1_upper = 8 + ((angle1 < 180 ? angle1 : 360 - angle1) * 5) / 180;
            int y2_upper = 8 + ((angle2 < 180 ? angle2 : 360 - angle2) * 5) / 180;
            
            int y1_lower = 16 + ((angle1 < 180 ? angle1 : 360 - angle1) * 5) / 180;
            int y2_lower = 16 + ((angle2 < 180 ? angle2 : 360 - angle2) * 5) / 180;
            
            if (y1_upper >= 0 && y1_upper < 25) {
                vga_set_color(VGA_CYAN, VGA_BLCK);
                vga_putchr_at(x, y1_upper, 'O');
            }
            if (y2_upper >= 0 && y2_upper < 25) {
                vga_set_color(VGA_MAGENTA, VGA_BLCK);
                vga_putchr_at(x, y2_upper, 'O');
            }
            
            if (y1_lower >= 0 && y1_lower < 25) {
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_putchr_at(x, y1_lower, 'O');
            }
            if (y2_lower >= 0 && y2_lower < 25) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
                vga_putchr_at(x, y2_lower, 'O');
            }
            
            if (x % 5 == 0) {
                int start = y1_upper < y2_upper ? y1_upper : y2_upper;
                int end = y1_upper > y2_upper ? y1_upper : y2_upper;
                for (int y = start + 1; y < end; y++) {
                    if (y >= 0 && y < 25) {
                        vga_set_color(VGA_LGREY, VGA_BLCK);
                        vga_putchr_at(x, y, '|');
                    }
                }
            }
            
            if (x % 5 == 0) {
                int start = y1_lower < y2_lower ? y1_lower : y2_lower;
                int end = y1_lower > y2_lower ? y1_lower : y2_lower;
                for (int y = start + 1; y < end; y++) {
                    if (y >= 0 && y < 25) {
                        vga_set_color(VGA_DGREY, VGA_BLCK);
                        vga_putchr_at(x, y, '|');
                    }
                }
            }
        }
        
        time++;
        for (volatile int i = 0; i < 1600000; i++);
    }
}

void art_scanner(void) {
    vga_clear();
    int running = 1;
    int scan_line = 0;
    int direction = 1;
    
    typedef struct { int x, y, brightness; } Pixel;
    Pixel pixels[100];
    
    for (int i = 0; i < 100; i++) {
        pixels[i].x = art_rand() % 80;
        pixels[i].y = art_rand() % 25;
        pixels[i].brightness = 0;
    }
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        for (int i = 0; i < 100; i++) {
            if (pixels[i].brightness > 0) {
                pixels[i].brightness--;
            }
        }
        
        for (int i = 0; i < 100; i++) {
            if (pixels[i].y == scan_line) {
                pixels[i].brightness = 100;
            }
        }
        
        vga_clear();
        
        for (int x = 0; x < 80; x++) {
            vga_set_color(VGA_LGREEN, VGA_BLCK);
            vga_putchr_at(x, scan_line, 0xC4);
        }
        
        for (int i = 0; i < 100; i++) {
            if (pixels[i].brightness > 0) {
                uint8_t color = pixels[i].brightness > 70 ? VGA_WHITE :
                               pixels[i].brightness > 40 ? VGA_YELLOW :
                               pixels[i].brightness > 20 ? VGA_CYAN : VGA_BLUE;
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(pixels[i].x, pixels[i].y, 0xDB);
            }
        }
        
        scan_line += direction;
        if (scan_line >= 24 || scan_line <= 0) {
            direction = -direction;
        }
        
        pit_delay_ms(100);
    }
}


void art_spiral_galaxy(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int arm = 0; arm < 5; arm++) {
            for (int i = 0; i < 120; i++) {
                int angle = (i * 8 + arm * 72 + time) % 360;
                int radius = i / 3;
                
                int curve = (i * 3) % 360;
                angle = (angle + curve / 4) % 360;
                
                int x = 40 + (radius * (angle < 90 || angle > 270 ? 1 : -1)) / 2;
                int y = 12 + (radius * (angle < 180 ? 1 : -1)) / 4;
                
                if (x >= 0 && x < 80 && y >= 0 && y < 25) {
                    uint8_t color;
                    char c;
                    
                    if (i < 20) { 
                        color = VGA_WHITE; c = '*'; 
                    }
                    else if (i < 50) { 
                        color = VGA_YELLOW; c = i % 3 == 0 ? '*' : '.';
                    }
                    else if (i < 80) { 
                        color = VGA_LRED; c = i % 4 == 0 ? '.' : ':';
                    }
                    else { 
                        color = VGA_RED; c = ':';
                    }
                    
                    vga_set_color(color, VGA_BLCK);
                    vga_putchr_at(x, y, c);
                }
            }
        }
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_putchr_at(40, 12, 0xDB);
        vga_putchr_at(39, 12, 0xDB);
        vga_putchr_at(41, 12, 0xDB);
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_putchr_at(40, 11, 0xDB);
        vga_putchr_at(40, 13, 0xDB);
        
        time++;
        for (volatile int i = 0; i < 600000; i++);
    }
}

void art_matrix_slide(void) {
    const char* chars = "01abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()";
    int drops[80];
    int speeds[80];
    
    for (int i = 0; i < 80; i++) {
        drops[i] = -(art_rand() % 25);
        speeds[i] = 1 + (art_rand() % 3);
    }
    
    vga_clear();
    int running = 1;
    int frame = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        if (frame % 2 == 0) {
            for (int y = 0; y < 25; y++) {
                for (int x = 0; x < 80; x++) {
                    if (art_rand() % 10 == 0) {
                        vga_putchr_at(x, y, ' ');
                    }
                }
            }
        }
        
        for (int x = 0; x < 80; x++) {
            if (frame % speeds[x] == 0) {
                drops[x]++;
                
                if (drops[x] >= 0 && drops[x] < 25) {
                    int idx = art_rand() % 66;
                    uint8_t color = (drops[x] == 0) ? VGA_WHITE : 
                                   (art_rand() % 3 == 0) ? VGA_LGREEN : VGA_GREEN;
                    vga_putchr_color(chars[idx], color, VGA_BLCK);
                    vga_t_column = x;
                    vga_t_row = drops[x];
                    vga_putchr(chars[idx]);
                }
                
                if (drops[x] > 30) {
                    drops[x] = -(art_rand() % 15);
                    speeds[x] = 1 + (art_rand() % 3);
                }
            }
        }
        
        frame++;
        pit_delay_ms(100);
    }
}

void art_square_waves(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    
    int sine_table[80];
    for (int i = 0; i < 80; i++) {
        int angle = (i * 360) / 80;
        if (angle < 90) sine_table[i] = (angle * 12) / 90;
        else if (angle < 180) sine_table[i] = 12 - ((angle - 90) * 12) / 90;
        else if (angle < 270) sine_table[i] = -((angle - 180) * 12) / 90;
        else sine_table[i] = -12 + ((angle - 270) * 12) / 90;
    }
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int wave = 0; wave < 5; wave++) {
            uint8_t color = VGA_BLUE + wave;
            
            for (int x = 0; x < 80; x++) {
                int offset = (x + time * (wave + 1)) % 80;
                int y = 12 + sine_table[offset] - (wave * 2);
                
                if (y >= 0 && y < 25) {
                    vga_set_color(color, VGA_BLCK);
                    vga_putchr_at(x, y, 0xDB);
                }
            }
        }
        
        time++;
        pit_delay_ms(100);
    }
}

void art_starfield_2(void) {
    typedef struct {
        int x, y, z;
        uint8_t color;
    } Star;
    
    Star stars[100];
    
    for (int i = 0; i < 100; i++) {
        stars[i].x = (art_rand() % 160) - 80;
        stars[i].y = (art_rand() % 50) - 25;
        stars[i].z = art_rand() % 100 + 1;
        stars[i].color = VGA_WHITE;
    }
    
    vga_clear();
    int running = 1;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        vga_clear();
        
        for (int i = 0; i < 100; i++) {
            stars[i].z--;
            if (stars[i].z <= 0) {
                stars[i].x = (art_rand() % 160) - 80;
                stars[i].y = (art_rand() % 50) - 25;
                stars[i].z = 100;
            }
            
            int sx = 40 + (stars[i].x * 40) / stars[i].z;
            int sy = 12 + (stars[i].y * 12) / stars[i].z;
            
            if (sx >= 0 && sx < 80 && sy >= 0 && sy < 25) {
                uint8_t color = stars[i].z > 70 ? VGA_DGREY :
                               stars[i].z > 40 ? VGA_LGREY : VGA_WHITE;
                vga_putchr_at(sx, sy, stars[i].z > 50 ? '.' : '*');
                vga_set_color(color, VGA_BLCK);
            }
        }
        
        pit_delay_ms(100);
    }
}

void art_tunnel(void) {
    vga_clear();
    int running = 1;
    int time = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 80; x++) {
                int dx = x - 40;
                int dy = (y - 12) * 2;
                
                int dist = 1;
                if (dx != 0 || dy != 0) {
                    dist = dx*dx + dy*dy;
                    if (dist > 100) dist = 100;
                }
                
                int value = (dist + time) % 20;
                
                uint8_t color;
                if (value < 5) color = VGA_BLUE;
                else if (value < 10) color = VGA_CYAN;
                else if (value < 15) color = VGA_GREEN;
                else color = VGA_YELLOW;
                
                char c = (value % 5 == 0) ? '#' : 0xB0;
                vga_putchr_at(x, y, c);
                vga_set_color(color, VGA_BLCK);
            }
        }
        
        time++;
        pit_delay_ms(100);
    }
}

void art_metaballs_2(void) {
    typedef struct { int x, y, vx, vy; } Ball;
    Ball balls[5];
    
    for (int i = 0; i < 5; i++) {
        balls[i].x = art_rand() % 80;
        balls[i].y = art_rand() % 25;
        balls[i].vx = (art_rand() % 3) - 1;
        balls[i].vy = (art_rand() % 3) - 1;
    }
    
    vga_clear();
    int running = 1;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        for (int i = 0; i < 5; i++) {
            balls[i].x += balls[i].vx;
            balls[i].y += balls[i].vy;
            
            if (balls[i].x <= 0 || balls[i].x >= 79) balls[i].vx = -balls[i].vx;
            if (balls[i].y <= 0 || balls[i].y >= 24) balls[i].vy = -balls[i].vy;
        }
        
        for (int y = 0; y < 25; y++) {
            for (int x = 0; x < 80; x++) {
                int sum = 0;
                
                for (int i = 0; i < 5; i++) {
                    int dx = x - balls[i].x;
                    int dy = y - balls[i].y;
                    int dist = dx*dx + dy*dy + 1;
                    sum += 100 / dist;
                }
                
                uint8_t color;
                if (sum < 10) color = VGA_BLCK;
                else if (sum < 20) color = VGA_BLUE;
                else if (sum < 30) color = VGA_CYAN;
                else if (sum < 40) color = VGA_GREEN;
                else if (sum < 50) color = VGA_YELLOW;
                else color = VGA_WHITE;
                
                vga_putchr_at(x, y, 0xDB);
                vga_set_color(color, VGA_BLCK);
            }
        }
        
        pit_delay_ms(100);
    }
}

void art_radar(void) {
    vga_clear();
    int running = 1;
    int angle = 0;
    
    typedef struct { int x, y, age; } Blip;
    Blip blips[20];
    
    for (int i = 0; i < 20; i++) {
        blips[i].x = (art_rand() % 60) - 30;
        blips[i].y = (art_rand() % 30) - 15;
        blips[i].age = 0;
    }
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            get_char();
            break;
        }
        
        if (angle % 4 == 0) {
            for (int i = 0; i < 50; i++) {
                int x = art_rand() % 80;
                int y = art_rand() % 25;
                vga_putchr_at(x, y, ' ');
            }
        }
        
        for (int a = 0; a < 360; a += 10) {
            for (int r = 5; r < 40; r += 10) {
                int x = 40 + (r * (a < 90 || a > 270 ? 1 : -1)) / 3;
                int y = 12 + (r * (a < 180 ? 1 : -1)) / 6;
                if (x >= 0 && x < 80 && y >= 0 && y < 25) {
                    vga_putchr_at(x, y, '.');
                    vga_set_color(VGA_GREEN, VGA_BLCK);
                }
            }
        }
        
        for (int r = 0; r < 40; r++) {
            int x = 40 + (r * (angle < 90 || angle > 270 ? 1 : -1)) / 3;
            int y = 12 + (r * (angle < 180 ? 1 : -1)) / 6;
            if (x >= 0 && x < 80 && y >= 0 && y < 25) {
                vga_putchr_at(x, y, '-');
                vga_set_color(VGA_LGREEN, VGA_BLCK);
            }
        }
        
        for (int i = 0; i < 20; i++) {
            int sx = 40 + blips[i].x;
            int sy = 12 + blips[i].y / 2;
            
            if (sx >= 0 && sx < 80 && sy >= 0 && sy < 25) {
                uint8_t color = blips[i].age < 10 ? VGA_WHITE :
                               blips[i].age < 20 ? VGA_LGREEN : VGA_GREEN;
                vga_putchr_at(sx, sy, 'X');
                vga_set_color(color, VGA_BLCK);
            }
            
            blips[i].age++;
            if (blips[i].age > 30) {
                blips[i].x = (art_rand() % 60) - 30;
                blips[i].y = (art_rand() % 30) - 15;
                blips[i].age = 0;
            }
        }
        
        angle = (angle + 5) % 360;
        for (volatile int i = 0; i < 7000000; i++);
    }
}


void art_gallery_main(void) {
    ArtPiece gallery[] = {
        {"Matrix Rain", "Digital rain like The Matrix", art_matrix_rain},
        {"Matrix Slide", "Digital slide like The Matrix",art_matrix_slide },
        {"Aurora Waves", "Flowing wave patterns", art_aurora_waves},
        {"Sinewaves", "Sqaure overlapping waves",art_square_waves},
        {"Lava Lamp", "Floating colorful bubbles", art_fire},
        {"Kaleidoscope", "Rotating geometric patterns", art_kaleidoscope},
        {"Starfield Monochrome", "Flying through the cosmos BW", art_starfield_2},
        {"Starfield Color", "Flying through the cosmos", art_starfield},
        {"Fractal Trees", "Recursive branch patterns", art_fractal},
        {"Tunnel", "3D tunnel effect", art_tunnel},
        {"Ripple Effect", "Concentric expanding rings", art_ripple},
        {"Radar", "Radar like graphics", art_radar},
        {"Metaballs Glow", "Glowing flowing shapes", art_metaballs_2},
        {"Metaballs", "Organic flowing shapes", art_metaballs},
        {"Fireworks", "Explosive particle fountain", art_particles},
        {"DNA Double Helix", "Two intertwined spirals", art_dna_helix},
        {"Scanner", "Matrix-style scan effect", art_scanner},
        {"Spiral Galaxy", "Rotating cosmic spiral", art_spiral_galaxy}
    };
    
    int selection = 0;
    int running = 1;
    
    while (running) {
        vga_begin_batch();
        vga_clear();
        
        vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
        vga_set_color(VGA_YELLOW, VGA_BLUE);
        vga_print_centered("SmolOS Abstract Art Gallery", 0);
        vga_set_color(VGA_LCYAN, VGA_BLUE);
        vga_print_centered("18 Stunning Visual Effects", 1);
        
        vga_draw_box_double(5, 3, 70, 18, VGA_CYAN, VGA_BLCK);
        
        for (int i = 0; i < 18; i++) {
            int y = 5 + i;
            int selected = (i == selection);
            
            if (selected) {
                vga_fill_rect(7, y, 66, 1, ' ', VGA_BLCK, VGA_CYAN);
            }
            
            vga_set_color(selected ? VGA_BLCK : VGA_WHITE, selected ? VGA_CYAN : VGA_BLCK);
            if (i < 9) {
                vga_putchr_at(8, y, ' ');
                vga_putchr_at(9, y, '0' + (i + 1));
            } else {
                vga_putchr_at(8, y, '1');
                vga_putchr_at(9, y, '0' + (i + 1 - 10));
            }
            vga_putchr_at(10, y, '.');
            vga_putchr_at(11, y, ' ');
            
            int x = 13;
            int name_idx = 0;
            while (gallery[i].name[name_idx] && x < 35) {
                vga_putchr_at(x, y, gallery[i].name[name_idx]);
                x++;
                name_idx++;
            }
            
            while (x < 35) {
                vga_putchr_at(x, y, ' ');
                x++;
            }
            
            vga_set_color(VGA_DGREY, selected ? VGA_CYAN : VGA_BLCK);
            vga_putchr_at(35, y, '-');
            vga_putchr_at(36, y, ' ');
            
            x = 37;
            int desc_idx = 0;
            while (gallery[i].description[desc_idx] && x < 73) {
                vga_putchr_at(x, y, gallery[i].description[desc_idx]);
                x++;
                desc_idx++;
            }
            
            while (x < 73) {
                vga_putchr_at(x, y, ' ');
                x++;
            }
        }
        
        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_print_centered("Use UP/DOWN arrows to select, ENTER to view, ESC to exit", 23);
        
        vga_end_batch();
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == 0x11) {  
                selection--;
                if (selection < 0) selection = 17;
            }
            else if (c == 0x12) { 
                selection++;
                if (selection > 17) selection = 0;
            }
            else if (c == '\n') {  
                vga_clear();
                art_init_random();
                gallery[selection].render_func();
                vga_clear();
            }
            else if (c == 27) {  
                running = 0;
            }
        }
        
        for (volatile int i = 0; i < 10000; i++);
    }
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}