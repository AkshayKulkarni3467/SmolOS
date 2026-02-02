#include "sos_lifesim.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_mouse.h"
#include "sos_pit.h"
#include "sos_memory.h"

static uint8_t grid[LIFE_HEIGHT][LIFE_WIDTH];
static uint8_t next_grid[LIFE_HEIGHT][LIFE_WIDTH];
static LifeState game_state;
static DrawMode draw_mode;
static int simulation_speed;
static uint32_t generation;
static uint32_t population;
static int menu_selection;
static int is_drawing;
static Point draw_start;
static int show_grid_lines;
static int color_mode;
static int wrap_edges;

static uint32_t life_rand_seed = 98765;

static uint32_t life_rand(void) {
    life_rand_seed = (life_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return life_rand_seed;
}

static const uint8_t glider_pattern[] = {
    0, 1, 0,
    0, 0, 1,
    1, 1, 1
};

static const uint8_t lwss_pattern[] = {
    0, 1, 0, 0, 1,
    1, 0, 0, 0, 0,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 0
};

static const uint8_t pulsar_pattern[] = {
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,0,0,0,1,1,1,0,0
};

static const Pattern patterns[] = {
    {"Glider", 3, 3, glider_pattern},
    {"LWSS", 5, 4, lwss_pattern},
    {"Pulsar", 13, 13, pulsar_pattern}
};

void life_init(void) {
    game_state = LIFE_MENU;
    draw_mode = DRAW_MODE_SINGLE;
    simulation_speed = 100;
    generation = 0;
    population = 0;
    menu_selection = 0;
    is_drawing = 0;
    show_grid_lines = 1;
    color_mode = 0;
    wrap_edges = 0;
    life_rand_seed = pit_get_ticks();
    
    mouse_show_cursor();
    mouse_set_cursor_char(0xFE);
    
    life_reset();
}

void life_reset(void) {
    for (int y = 0; y < LIFE_HEIGHT; y++) {
        for (int x = 0; x < LIFE_WIDTH; x++) {
            grid[y][x] = 0;
            next_grid[y][x] = 0;
        }
    }
    generation = 0;
    population = 0;
}

int life_get_cell(int x, int y) {
    if (wrap_edges) {
        x = (x + LIFE_WIDTH) % LIFE_WIDTH;
        y = (y + LIFE_HEIGHT) % LIFE_HEIGHT;
        return grid[y][x];
    } else {
        if (x < 0 || x >= LIFE_WIDTH || y < 0 || y >= LIFE_HEIGHT) {
            return 0;
        }
        return grid[y][x];
    }
}

void life_set_cell(int x, int y, int alive) {
    if (x >= 0 && x < LIFE_WIDTH && y >= 0 && y < LIFE_HEIGHT) {
        if (grid[y][x] != alive) {
            grid[y][x] = alive;
            if (alive) population++;
            else population--;
        }
    }
}

void life_toggle_cell(int x, int y) {
    if (x >= 0 && x < LIFE_WIDTH && y >= 0 && y < LIFE_HEIGHT) {
        grid[y][x] = !grid[y][x];
        if (grid[y][x]) population++;
        else population--;
    }
}

int life_count_neighbors(int x, int y) {
    int count = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (wrap_edges) {
                nx = (nx + LIFE_WIDTH) % LIFE_WIDTH;
                ny = (ny + LIFE_HEIGHT) % LIFE_HEIGHT;
                count += grid[ny][nx];
            } else {
                if (nx >= 0 && nx < LIFE_WIDTH && ny >= 0 && ny < LIFE_HEIGHT) {
                    count += grid[ny][nx];
                }
            }
        }
    }
    
    return count;
}

void life_update(void) {
    population = 0;
    
    for (int y = 0; y < LIFE_HEIGHT; y++) {
        for (int x = 0; x < LIFE_WIDTH; x++) {
            int neighbors = life_count_neighbors(x, y);
            int cell = grid[y][x];
            
            if (cell) {
                next_grid[y][x] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                next_grid[y][x] = (neighbors == 3) ? 1 : 0;
            }
            
            if (next_grid[y][x]) population++;
        }
    }
    
    for (int y = 0; y < LIFE_HEIGHT; y++) {
        for (int x = 0; x < LIFE_WIDTH; x++) {
            grid[y][x] = next_grid[y][x];
        }
    }
    
    generation++;
}

void life_clear_grid(void) {
    for (int y = 0; y < LIFE_HEIGHT; y++) {
        for (int x = 0; x < LIFE_WIDTH; x++) {
            grid[y][x] = 0;
        }
    }
    generation = 0;
    population = 0;
}

void life_randomize(int density) {
    life_clear_grid();
    
    for (int y = 0; y < LIFE_HEIGHT; y++) {
        for (int x = 0; x < LIFE_WIDTH; x++) {
            if ((life_rand() % 100) < density) {
                grid[y][x] = 1;
                population++;
            }
        }
    }
}

void life_draw_pattern(int x, int y, const Pattern* pattern) {
    for (int py = 0; py < pattern->height; py++) {
        for (int px = 0; px < pattern->width; px++) {
            int idx = py * pattern->width + px;
            if (pattern->pattern[idx]) {
                life_set_cell(x + px, y + py, 1);
            }
        }
    }
}

void life_draw_glider(int x, int y) {
    life_draw_pattern(x, y, &patterns[0]);
}

void life_draw_lwss(int x, int y) {
    life_draw_pattern(x, y, &patterns[1]);
}

void life_draw_pulsar(int x, int y) {
    life_draw_pattern(x, y, &patterns[2]);
}

void life_draw_gosper_gun(int x, int y) {
    int gun[][2] = {
        {24, 0}, {22, 1}, {24, 1}, {12, 2}, {13, 2}, {20, 2}, {21, 2},
        {34, 2}, {35, 2}, {11, 3}, {15, 3}, {20, 3}, {21, 3}, {34, 3},
        {35, 3}, {0, 4}, {1, 4}, {10, 4}, {16, 4}, {20, 4}, {21, 4},
        {0, 5}, {1, 5}, {10, 5}, {14, 5}, {16, 5}, {17, 5}, {22, 5},
        {24, 5}, {10, 6}, {16, 6}, {24, 6}, {11, 7}, {15, 7}, {12, 8},
        {13, 8}
    };
    
    for (int i = 0; i < 36; i++) {
        life_set_cell(x + gun[i][0], y + gun[i][1], 1);
    }
}

void life_spray_paint(int x, int y) {
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if ((life_rand() % 100) < 40) {
                life_set_cell(x + dx, y + dy, 1);
            }
        }
    }
}

void life_draw_grid(void) {
    vga_begin_batch();
    
    for (int y = 0; y < LIFE_HEIGHT; y++) {
        for (int x = 0; x < LIFE_WIDTH; x++) {
            int screen_x = LIFE_OFFSET_X + x;
            int screen_y = LIFE_OFFSET_Y + y;
            
            uint8_t fg, bg;
            char c;
            
            if (grid[y][x]) {
                if (color_mode) {
                    int age = generation % 8;
                    uint8_t colors[] = {VGA_LGREEN, VGA_GREEN, VGA_LCYAN, VGA_CYAN, 
                                       VGA_YELLOW, VGA_LRED, VGA_RED, VGA_MAGENTA};
                    fg = colors[age];
                } else {
                    fg = VGA_LGREEN;
                }
                bg = VGA_BLCK;
                c = 0xDB;
            } else {
                if (show_grid_lines) {
                    fg = VGA_DGREY;
                    bg = VGA_BLCK;
                    c = 0xFA;
                } else {
                    fg = VGA_BLCK;
                    bg = VGA_BLCK;
                    c = ' ';
                }
            }
            
            vga_set_color(fg, bg);
            vga_putchr_at(screen_x, screen_y, c);
        }
    }
    
    vga_end_batch();
}

void life_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("CONWAY'S LIFE SIMULATOR", 0);
    
    vga_set_color(VGA_WHITE, VGA_BLUE);
    char info[80];
    int pos = 0;
    
    const char* gen_label = "Gen: ";
    for (int i = 0; gen_label[i]; i++) info[pos++] = gen_label[i];
    
    int temp = generation;
    if (temp == 0) {
        info[pos++] = '0';
    } else {
        char digits[10];
        int dc = 0;
        while (temp > 0) {
            digits[dc++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = dc - 1; i >= 0; i--) {
            info[pos++] = digits[i];
        }
    }
    
    info[pos++] = ' ';
    info[pos++] = '|';
    info[pos++] = ' ';
    
    const char* pop_label = "Pop: ";
    for (int i = 0; pop_label[i]; i++) info[pos++] = pop_label[i];
    
    temp = population;
    if (temp == 0) {
        info[pos++] = '0';
    } else {
        char digits[10];
        int dc = 0;
        while (temp > 0) {
            digits[dc++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = dc - 1; i >= 0; i--) {
            info[pos++] = digits[i];
        }
    }
    
    info[pos++] = ' ';
    info[pos++] = '|';
    info[pos++] = ' ';
    
    const char* speed_label = "Speed: ";
    for (int i = 0; speed_label[i]; i++) info[pos++] = speed_label[i];
    
    temp = simulation_speed;
    if (temp == 0) {
        info[pos++] = '0';
    } else {
        char digits[10];
        int dc = 0;
        while (temp > 0) {
            digits[dc++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = dc - 1; i >= 0; i--) {
            info[pos++] = digits[i];
        }
    }
    
    const char* ms = "ms";
    for (int i = 0; ms[i]; i++) info[pos++] = ms[i];
    
    info[pos] = '\0';
    
    for (int i = 0; i < pos; i++) {
        vga_putchr_at(2 + i, 1, info[i]);
    }
    
    vga_draw_box_single(0, 24, 40, 1, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    
    const char* mode_names[] = {
        "Single", "Spray", "Line", "Rect", "Glider", 
        "LWSS", "Pulsar", "G-Gun", "Erase"
    };
    
    const char* mode_label = "Mode: ";
    int x = 2;
    for (int i = 0; mode_label[i]; i++) {
        vga_putchr_at(x++, 24, mode_label[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    for (int i = 0; mode_names[draw_mode][i]; i++) {
        vga_putchr_at(x++, 24, mode_names[draw_mode][i]);
    }
    
    vga_draw_box_single(40, 24, 40, 1, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    const char* state_text = game_state == LIFE_RUNNING ? "RUNNING" : "PAUSED";
    uint8_t state_color = game_state == LIFE_RUNNING ? VGA_LGREEN : VGA_YELLOW;
    
    x = 42;
    vga_set_color(state_color, VGA_BLCK);
    for (int i = 0; state_text[i]; i++) {
        vga_putchr_at(x++, 24, state_text[i]);
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls = " | M:Mode C:Clear R:Random";
    for (int i = 0; controls[i]; i++) {
        vga_putchr_at(x++, 24, controls[i]);
    }
}

void life_draw_menu(void) {
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* title[] = {
        " _     _  __                _____ _           ",
        "| |   (_)/ _| ___          / ____(_)          ",
        "| |    _| |_ / _ \\  ______| (___  _ _ __ ___  ",
        "| |   | |  _|  __/ |______|\___ \\| | '_ ` _ \\ ",
        "| |___| | |  \\___|         ____) | | | | | | |",
        "|_____|_|_|                |_____/|_|_| |_| |_|"
    };
    
    for (int i = 0; i < 6; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        uint8_t colors[] = {VGA_LCYAN, VGA_CYAN, VGA_LGREEN, VGA_GREEN, VGA_YELLOW, VGA_LRED};
        vga_set_color(colors[i], VGA_BLCK);
        
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 4 + i, title[i][j]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* subtitle = "Interactive Conway's Game of Life";
    int len = 0;
    while (subtitle[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 11, subtitle[i]);
    }
    
    const char* options[] = {
        "Start Simulation",
        "Random Start (30%)",
        "Random Start (50%)",
        "View Help",
        "Exit"
    };
    
    for (int i = 0; i < 5; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(26, 14 + i * 2, 28, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 28;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 14 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 14 + i * 2, options[i][j]);
        }
    }
}

void life_draw_help(void) {
    vga_clear();
    
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_print_centered("HELP & CONTROLS", 0);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* sections[] = {
        "MOUSE CONTROLS:",
        "  Left Click: Draw cells (mode-dependent)",
        "  Right Click: Erase cells",
        "  Click & Drag: Continuous drawing",
        "",
        "KEYBOARD CONTROLS:",
        "  SPACE: Play/Pause simulation",
        "  M: Cycle through draw modes",
        "  C: Clear grid",
        "  R: Randomize grid (30% density)",
        "  +/-: Increase/Decrease speed",
        "  G: Toggle grid lines",
        "  W: Toggle edge wrapping",
        "  H: Return to this help",
        "  ESC: Return to menu",
        "",
        "DRAW MODES:",
        "  Single: Draw individual cells",
        "  Spray: Paint with random pattern",
        "  Glider: Place glider pattern",
        "  LWSS: Place lightweight spaceship",
        "  Pulsar: Place pulsar oscillator",
        "  G-Gun: Gosper glider gun",
        "",
        "Press any key to continue..."
    };
    
    int y = 2;
    for (int i = 0; i < 24; i++) {
        int len = 0;
        while (sections[i][len]) len++;
        
        uint8_t color = VGA_WHITE;
        if (sections[i][0] != ' ' && sections[i][0] != '\0') {
            color = VGA_YELLOW;
        }
        
        vga_set_color(color, VGA_BLCK);
        for (int j = 0; j < len; j++) {
            vga_putchr_at(5 + j, y, sections[i][j]);
        }
        y++;
    }
    
    wait_for_char();
}

void life_handle_mouse(void) {
    MouseEvent event;
    
    while (mouse_has_event()) {
        mouse_get_event(&event);
        
        int mouse_x, mouse_y;
        mouse_get_position(&mouse_x, &mouse_y);
        
        int grid_x = mouse_x - LIFE_OFFSET_X;
        int grid_y = mouse_y - LIFE_OFFSET_Y;
        
        if (grid_x >= 0 && grid_x < LIFE_WIDTH && grid_y >= 0 && grid_y < LIFE_HEIGHT) {
            if (event.type == MOUSE_EVENT_BUTTON_PRESS) {
                if (event.buttons & MOUSE_LEFT_BUTTON) {
                    is_drawing = 1;
                    draw_start.x = grid_x;
                    draw_start.y = grid_y;
                    
                    switch (draw_mode) {
                        case DRAW_MODE_SINGLE:
                            life_set_cell(grid_x, grid_y, 1);
                            break;
                        case DRAW_MODE_SPRAY:
                            life_spray_paint(grid_x, grid_y);
                            break;
                        case DRAW_MODE_GLIDER:
                            life_draw_glider(grid_x, grid_y);
                            break;
                        case DRAW_MODE_LWSS:
                            life_draw_lwss(grid_x, grid_y);
                            break;
                        case DRAW_MODE_PULSAR:
                            life_draw_pulsar(grid_x, grid_y);
                            break;
                        case DRAW_MODE_GOSPER_GLIDER_GUN:
                            life_draw_gosper_gun(grid_x, grid_y);
                            break;
                        case DRAW_MODE_ERASE:
                            life_set_cell(grid_x, grid_y, 0);
                            break;
                        default:
                            break;
                    }
                }
                
                if (event.buttons & MOUSE_RIGHT_BUTTON) {
                    life_set_cell(grid_x, grid_y, 0);
                }
            } else if (event.type == MOUSE_EVENT_MOVE) {
                if (is_drawing && (event.buttons & MOUSE_LEFT_BUTTON)) {
                    if (draw_mode == DRAW_MODE_SINGLE || draw_mode == DRAW_MODE_ERASE) {
                        life_set_cell(grid_x, grid_y, draw_mode != DRAW_MODE_ERASE);
                    } else if (draw_mode == DRAW_MODE_SPRAY) {
                        life_spray_paint(grid_x, grid_y);
                    }
                }
            } else if (event.type == MOUSE_EVENT_BUTTON_RELEASE) {
                is_drawing = 0;
            }
        }
    }
}

void life_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == LIFE_MENU) {
        if (c == 0x11) {
            menu_selection = (menu_selection - 1 + 5) % 5;
        } else if (c == 0x12) {
            menu_selection = (menu_selection + 1) % 5;
        } else if (c == '\n') {
            if (menu_selection == 0) {
                game_state = LIFE_PAUSED;
            } else if (menu_selection == 1) {
                life_randomize(30);
                game_state = LIFE_PAUSED;
            } else if (menu_selection == 2) {
                life_randomize(50);
                game_state = LIFE_PAUSED;
            } else if (menu_selection == 3) {
                life_draw_help();
            } else if (menu_selection == 4) {
                return;
            }
        } else if (c == 27) {
            return;
        }
    } else if (game_state == LIFE_RUNNING || game_state == LIFE_PAUSED) {
        if (c == ' ') {
            game_state = (game_state == LIFE_RUNNING) ? LIFE_PAUSED : LIFE_RUNNING;
        } else if (c == 'm' || c == 'M') {
            draw_mode = (draw_mode + 1) % 9;
        } else if (c == 'c' || c == 'C') {
            life_clear_grid();
        } else if (c == 'r' || c == 'R') {
            life_randomize(30);
        } else if (c == '+' || c == '=') {
            if (simulation_speed > 20) simulation_speed -= 10;
        } else if (c == '-' || c == '_') {
            if (simulation_speed < 500) simulation_speed += 10;
        } else if (c == 'g' || c == 'G') {
            show_grid_lines = !show_grid_lines;
        } else if (c == 'w' || c == 'W') {
            wrap_edges = !wrap_edges;
        } else if (c == 'h' || c == 'H') {
            life_draw_help();
        } else if (c == 'v' || c == 'V') {
            color_mode = !color_mode;
        } else if (c == 27) {
            game_state = LIFE_MENU;
        }
    }
}

void life_game_run(void) {
    life_init();
    
    uint32_t last_update = 0;
    
    while (1) {
        life_handle_input();
        life_handle_mouse();
        
        if (game_state == LIFE_MENU) {
            life_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == LIFE_RUNNING || game_state == LIFE_PAUSED) {
            uint32_t current_time = pit_get_total_milliseconds();
            
            if (game_state == LIFE_RUNNING && 
                (current_time - last_update >= simulation_speed)) {
                life_update();
                last_update = current_time;
            }
            
            vga_clear();
            life_draw_ui();
            life_draw_grid();
            
            pit_delay_ms(16);
        }
        
        if (game_state == LIFE_MENU && menu_selection == 4) {
            break;
        }
    }
}

void life_game_cleanup(void) {
    mouse_hide_cursor();
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}