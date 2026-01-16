#include "sos_2048.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_2048_SAVE_FILE "2048SAVE.DAT"

static Tile2048 grid[GRID_SIZE][GRID_SIZE];
static Game2048State game_state;
static uint32_t score;
static uint32_t high_score;
static int menu_selection;
static int has_won;
static int continue_after_win;
static int needs_redraw;

static uint32_t rand_seed = 54321;

static uint32_t game_2048_rand(void) {
    rand_seed = (rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return rand_seed;
}


static void game_2048_draw_tile_at(int screen_x, int screen_y, int value, int is_new, int is_merged) {
    TileColor color = game_2048_get_tile_color(value);
    

    if (value == 0) {
        vga_fill_rect(screen_x, screen_y, TILE_WIDTH, TILE_HEIGHT, ' ', VGA_DGREY, VGA_DGREY);
        vga_draw_box_single(screen_x, screen_y, TILE_WIDTH, TILE_HEIGHT, VGA_DGREY, VGA_BLCK);
        return;
    }
    

    uint8_t display_bg = color.bg;
    uint8_t display_fg = color.fg;
    

    vga_fill_rect(screen_x, screen_y, TILE_WIDTH, TILE_HEIGHT, ' ', display_fg, display_bg);
    

    uint8_t border_color = is_merged ? VGA_YELLOW : color.fg;
    vga_draw_box_single(screen_x, screen_y, TILE_WIDTH, TILE_HEIGHT, border_color, VGA_BLCK);
    

    char num_str[8];
    int pos = 0;
    int temp = value;
    
    if (temp == 0) {
        num_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            num_str[pos++] = digits[i];
        }
    }
    num_str[pos] = '\0';
    

    int len = pos;
    int text_x = screen_x + (TILE_WIDTH - len) / 2;
    int text_y = screen_y + TILE_HEIGHT / 2;
    

    vga_set_color(color.fg, display_bg);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(text_x + i, text_y, num_str[i]);
    }
    

    if (is_merged && screen_x > 0 && screen_y > 0) {
        vga_set_color(VGA_YELLOW, display_bg);
        vga_putchr_at(screen_x + 1, screen_y + 1, 0x0F);
    }
}

TileColor game_2048_get_tile_color(int value) {
    TileColor color;
    
    switch (value) {
        case 0:
            color.fg = VGA_LGREY;
            color.bg = VGA_DGREY;
            break;
        case 2:
            color.fg = VGA_BLCK;
            color.bg = VGA_LGREY;
            break;
        case 4:
            color.fg = VGA_BLCK;
            color.bg = VGA_BRWN;
            break;
        case 8:
            color.fg = VGA_WHITE;
            color.bg = VGA_LRED;
            break;
        case 16:
            color.fg = VGA_WHITE;
            color.bg = VGA_RED;
            break;
        case 32:
            color.fg = VGA_WHITE;
            color.bg = VGA_MAGENTA;
            break;
        case 64:
            color.fg = VGA_WHITE;
            color.bg = VGA_LMAGENTA;
            break;
        case 128:
            color.fg = VGA_BLCK;
            color.bg = VGA_YELLOW;
            break;
        case 256:
            color.fg = VGA_BLCK;
            color.bg = VGA_YELLOW;
            break;
        case 512:
            color.fg = VGA_BLCK;
            color.bg = VGA_YELLOW;
            break;
        case 1024:
            color.fg = VGA_WHITE;
            color.bg = VGA_LGREEN;
            break;
        case 2048:
            color.fg = VGA_YELLOW;
            color.bg = VGA_LGREEN;
            break;
        default:
            if (value > 2048) {
                color.fg = VGA_YELLOW;
                color.bg = VGA_LCYAN;
            } else {
                color.fg = VGA_WHITE;
                color.bg = VGA_DGREY;
            }
            break;
    }
    
    return color;
}


void game_2048_init(void) {
    game_state = GAME_2048_MENU;
    menu_selection = 0;
    high_score = 0;
    has_won = 0;
    continue_after_win = 0;
    needs_redraw = 1;
    game_2048_load_high_score();
}

void game_2048_reset(void) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].value = 0;
            grid[i][j].merged = 0;
            grid[i][j].is_new = 0;
            grid[i][j].prev_x = j;
            grid[i][j].prev_y = i;
        }
    }
    
    score = 0;
    has_won = 0;
    continue_after_win = 0;
    needs_redraw = 1;
    
    game_2048_spawn_tile();
    game_2048_spawn_tile();
}

void game_2048_spawn_tile(void) {
    int empty_count = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j].value == 0) {
                empty_count++;
            }
        }
    }
    
    if (empty_count == 0) return;
    
    int target = game_2048_rand() % empty_count;
    int current = 0;
    
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j].value == 0) {
                if (current == target) {
                    grid[i][j].value = (game_2048_rand() % 10 < 9) ? 2 : 4;
                    grid[i][j].is_new = 1;
                    grid[i][j].merged = 0;
                    return;
                }
                current++;
            }
        }
    }
}

int game_2048_move(Direction2048 dir) {
    int moved = 0;
    

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j].merged = 0;
        }
    }
    
    if (dir == DIR_2048_LEFT) {
        for (int i = 0; i < GRID_SIZE; i++) {
            int write_pos = 0;
            for (int j = 0; j < GRID_SIZE; j++) {
                if (grid[i][j].value != 0) {
                    if (write_pos != j) {
                        grid[i][write_pos] = grid[i][j];
                        grid[i][j].value = 0;
                        moved = 1;
                    }
                    write_pos++;
                }
            }
            
            for (int j = 0; j < GRID_SIZE - 1; j++) {
                if (grid[i][j].value != 0 && grid[i][j].value == grid[i][j + 1].value) {
                    grid[i][j].value *= 2;
                    grid[i][j].merged = 1;
                    score += grid[i][j].value;
                    
                    if (grid[i][j].value == 2048 && !has_won) {
                        has_won = 1;
                        game_state = GAME_2048_WON;
                    }
                    
                    for (int k = j + 1; k < GRID_SIZE - 1; k++) {
                        grid[i][k] = grid[i][k + 1];
                    }
                    grid[i][GRID_SIZE - 1].value = 0;
                    moved = 1;
                }
            }
        }
    } else if (dir == DIR_2048_RIGHT) {
        for (int i = 0; i < GRID_SIZE; i++) {
            int write_pos = GRID_SIZE - 1;
            for (int j = GRID_SIZE - 1; j >= 0; j--) {
                if (grid[i][j].value != 0) {
                    if (write_pos != j) {
                        grid[i][write_pos] = grid[i][j];
                        grid[i][j].value = 0;
                        moved = 1;
                    }
                    write_pos--;
                }
            }
            
            for (int j = GRID_SIZE - 1; j > 0; j--) {
                if (grid[i][j].value != 0 && grid[i][j].value == grid[i][j - 1].value) {
                    grid[i][j].value *= 2;
                    grid[i][j].merged = 1;
                    score += grid[i][j].value;
                    
                    if (grid[i][j].value == 2048 && !has_won) {
                        has_won = 1;
                        game_state = GAME_2048_WON;
                    }
                    
                    for (int k = j - 1; k > 0; k--) {
                        grid[i][k] = grid[i][k - 1];
                    }
                    grid[i][0].value = 0;
                    moved = 1;
                }
            }
        }
    } else if (dir == DIR_2048_UP) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int write_pos = 0;
            for (int i = 0; i < GRID_SIZE; i++) {
                if (grid[i][j].value != 0) {
                    if (write_pos != i) {
                        grid[write_pos][j] = grid[i][j];
                        grid[i][j].value = 0;
                        moved = 1;
                    }
                    write_pos++;
                }
            }
            
            for (int i = 0; i < GRID_SIZE - 1; i++) {
                if (grid[i][j].value != 0 && grid[i][j].value == grid[i + 1][j].value) {
                    grid[i][j].value *= 2;
                    grid[i][j].merged = 1;
                    score += grid[i][j].value;
                    
                    if (grid[i][j].value == 2048 && !has_won) {
                        has_won = 1;
                        game_state = GAME_2048_WON;
                    }
                    
                    for (int k = i + 1; k < GRID_SIZE - 1; k++) {
                        grid[k][j] = grid[k + 1][j];
                    }
                    grid[GRID_SIZE - 1][j].value = 0;
                    moved = 1;
                }
            }
        }
    } else if (dir == DIR_2048_DOWN) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int write_pos = GRID_SIZE - 1;
            for (int i = GRID_SIZE - 1; i >= 0; i--) {
                if (grid[i][j].value != 0) {
                    if (write_pos != i) {
                        grid[write_pos][j] = grid[i][j];
                        grid[i][j].value = 0;
                        moved = 1;
                    }
                    write_pos--;
                }
            }

            for (int i = GRID_SIZE - 1; i > 0; i--) {
                if (grid[i][j].value != 0 && grid[i][j].value == grid[i - 1][j].value) {
                    grid[i][j].value *= 2;
                    grid[i][j].merged = 1;
                    score += grid[i][j].value;
                    
                    if (grid[i][j].value == 2048 && !has_won) {
                        has_won = 1;
                        game_state = GAME_2048_WON;
                    }
                    
                    for (int k = i - 1; k > 0; k--) {
                        grid[k][j] = grid[k - 1][j];
                    }
                    grid[0][j].value = 0;
                    moved = 1;
                }
            }
        }
    }
    
    return moved;
}

int game_2048_can_move(void) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j].value == 0) return 1;
        }
    }
    
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (j < GRID_SIZE - 1 && grid[i][j].value == grid[i][j + 1].value) return 1;
            if (i < GRID_SIZE - 1 && grid[i][j].value == grid[i + 1][j].value) return 1;
        }
    }
    return 0;
}


void game_2048_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == GAME_2048_MENU) {
        if (c == 0x11) {  
            menu_selection = (menu_selection - 1 + 3) % 3;
            needs_redraw = 1;
        } else if (c == 0x12) {  
            menu_selection = (menu_selection + 1) % 3;
            needs_redraw = 1;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                game_2048_reset();
                game_state = GAME_2048_PLAYING;
                needs_redraw = 1;
            } else if (menu_selection == 1) {

            } else if (menu_selection == 2) {
                game_state = GAME_2048_OVER;
            }
        } else if (c == 27) {  
            game_state = GAME_2048_OVER;
        }
    } else if (game_state == GAME_2048_PLAYING) {
        Direction2048 dir = DIR_2048_NONE;
        
        if (c == 0x11 || c == 'w' || c == 'W') {
            dir = DIR_2048_UP;
        } else if (c == 0x12 || c == 's' || c == 'S') {
            dir = DIR_2048_DOWN;
        } else if (c == 0x13 || c == 'a' || c == 'A') {
            dir = DIR_2048_LEFT;
        } else if (c == 0x14 || c == 'd' || c == 'D') {
            dir = DIR_2048_RIGHT;
        } else if (c == 'p' || c == 'P' || c == ' ') {
            game_state = GAME_2048_PAUSED;
            needs_redraw = 1;
        } else if (c == 27) {  
            game_state = GAME_2048_MENU;
            needs_redraw = 1;
        }
        
        if (dir != DIR_2048_NONE) {
            int moved = game_2048_move(dir);
            
            if (moved) {
                game_2048_spawn_tile();
                needs_redraw = 1;
                
                if (!game_2048_can_move()) {
                    if (score > high_score) {
                        high_score = score;
                        game_2048_save_high_score();
                    }
                    game_state = GAME_2048_OVER;
                }
            }
        }
    } else if (game_state == GAME_2048_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = GAME_2048_PLAYING;
            needs_redraw = 1;
        } else if (c == 27) {  
            game_state = GAME_2048_MENU;
            needs_redraw = 1;
        }
    } else if (game_state == GAME_2048_WON) {
        if (c == '\n' || c == ' ') {
            continue_after_win = 1;
            game_state = GAME_2048_PLAYING;
            needs_redraw = 1;
        } else if (c == 27 || c == 'n' || c == 'N') {
            if (score > high_score) {
                high_score = score;
                game_2048_save_high_score();
            }
            game_state = GAME_2048_MENU;
            needs_redraw = 1;
        }
    }
}


void game_2048_draw_grid(void) {
    static int first_draw = 1;
    if (!first_draw) {
        for (int i = 0; i < GRID_SIZE; i++) {
            for (int j = 0; j < GRID_SIZE; j++) {
                if (grid[i][j].is_new) {
                    grid[i][j].is_new = 0;
                }
            }
        }
    }
    first_draw = 0;

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* grid_title = "[ GRID ]";
    int len = 0;
    while (grid_title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(GRID_OFFSET_X + 14 + i, GRID_OFFSET_Y - 1, grid_title[i]);
    }

    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            int screen_x = GRID_OFFSET_X + j * (TILE_WIDTH + 1);
            int screen_y = GRID_OFFSET_Y + i * (TILE_HEIGHT + 1);

            game_2048_draw_tile_at(
                screen_x,
                screen_y,
                grid[i][j].value,
                grid[i][j].is_new,
                grid[i][j].merged
            );
        }
    }
}

void game_2048_draw_ui(void) {
    char buf[16];
    int pos, temp;

    vga_fill_rect(0, 0, 80, 2, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== 2048 GAME ===", 0);
    vga_set_color(VGA_LCYAN, VGA_BLUE);
    vga_print_centered("Join the numbers to reach 2048!", 1);

    vga_draw_box_single(2, 3, 18, 3, VGA_YELLOW, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* score_label = "SCORE";
    for (int i = 0; score_label[i]; i++) {
        vga_putchr_at(4 + i, 3, score_label[i]);
    }

    pos = 0;
    temp = score;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp > 0) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    vga_set_color(VGA_LGREEN, VGA_BLCK);
    for (int i = 0; buf[i]; i++)
        vga_putchr_at(4 + i, 4, buf[i]);

    vga_draw_box_single(60, 3, 18, 3, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_CYAN, VGA_BLCK);
    const char* high_label = "BEST";
    for (int i = 0; high_label[i]; i++) {
        vga_putchr_at(62 + i, 3, high_label[i]);
    }

    pos = 0;
    temp = high_score;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp > 0) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    vga_set_color(VGA_LMAGENTA, VGA_BLCK);
    for (int i = 0; buf[i]; i++)
        vga_putchr_at(62 + i, 4, buf[i]);

    int max_tile = 0;
    for (int y = 0; y < GRID_SIZE; y++)
        for (int x = 0; x < GRID_SIZE; x++)
            if (grid[y][x].value > max_tile)
                max_tile = grid[y][x].value;

    vga_draw_box_single(2, 20, 18, 3, VGA_LCYAN, VGA_BLCK);
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* max_label = "MAX TILE";
    for (int i = 0; max_label[i]; i++)
        vga_putchr_at(4 + i, 20, max_label[i]);

    pos = 0;
    temp = max_tile;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp > 0) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    for (int i = 0; buf[i]; i++)
        vga_putchr_at(4 + i, 21, buf[i]);

    vga_draw_box_double(2, 22, 76, 3, VGA_LGREY, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* c1 = "ARROWS/WASD:Move  SPACE/P:Pause  ESC:Menu";
    for (int i = 0; c1[i]; i++) 
        vga_putchr_at(4 + i, 23, c1[i]);
}

void game_2048_draw_menu(void) {
    vga_set_auto_swap(0);
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* title[] = {
        " ____   ___  _  _   ___  ",
        "|___ \\ / _ \\| || | / _ \\ ",
        "  __) | | | | || || (_) |",
        " / __/| |_| |__   _\\__, |",
        "|_____|\\___/   |_|   /_/ "
    };
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        uint8_t colors[] = {VGA_LCYAN, VGA_CYAN, VGA_LGREEN, VGA_YELLOW, VGA_LRED};
        vga_set_color(colors[i], VGA_BLCK);
        
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 4 + i, title[i][j]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* subtitle = "Join the numbers to reach 2048!";
    int len = 0;
    while (subtitle[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 10, subtitle[i]);
    }
    
    const char* options[] = {
        "Play Game",
        "View High Score",
        "Exit to OS"
    };
    
    for (int i = 0; i < 3; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_fill_rect(26, 13 + i * 2, 28, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 28;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 13 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 13 + i * 2, options[i][j]);
        }
    }
    
    vga_draw_box_double(22, 20, 36, 3, VGA_YELLOW, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* hs_label = "Best Score: ";
    int x = 25;
    for (int i = 0; hs_label[i]; i++) {
        vga_putchr_at(x++, 21, hs_label[i]);
    }
    
    char hs_str[16];
    int pos = 0;
    int temp = high_score;
    if (temp == 0) {
        hs_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            hs_str[pos++] = digits[i];
        }
    }
    hs_str[pos] = '\0';
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    for (int i = 0; i < pos; i++) {
        vga_putchr_at(x++, 21, hs_str[i]);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use arrow keys or WASD to move tiles";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void game_2048_draw_pause_screen(void) {
    vga_set_auto_swap(0);
    
    vga_draw_box_double(25, 9, 30, 7, VGA_YELLOW, VGA_BLCK);
    vga_fill_rect(26, 10, 28, 5, ' ', VGA_YELLOW, VGA_DGREY);
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* pause_text = "** PAUSED **";
    int len = 0;
    while (pause_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 11, pause_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_DGREY);
    const char* inst = "Press P to continue";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, inst[i]);
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void game_2048_draw_win_screen(void) {
    vga_set_auto_swap(0);
    
    vga_draw_box_double(15, 8, 50, 9, VGA_LGREEN, VGA_BLCK);
    vga_fill_rect(16, 9, 48, 7, ' ', VGA_YELLOW, VGA_LGREEN);

    vga_set_color(VGA_YELLOW, VGA_LGREEN);
    const char* win_text = "** YOU WIN! **";
    int len = 0;
    while (win_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, win_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_LGREEN);
    const char* congrats = "You reached 2048!";
    len = 0;
    while (congrats[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 12, congrats[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_LGREEN);
    vga_putchr_at(20, 10, 0x0F);
    vga_putchr_at(59, 10, 0x0F);
    vga_putchr_at(23, 12, 0x0F);
    vga_putchr_at(56, 12, 0x0F);
    
    vga_set_color(VGA_LGREY, VGA_LGREEN);
    const char* inst1 = "ENTER: Continue playing";
    const char* inst2 = "ESC/N: Return to menu";
    
    len = 0;
    while (inst1[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 14, inst1[i]);
    }
    
    len = 0;
    while (inst2[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, inst2[i]);
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void game_2048_draw_game_over(void) {
    char buf[16];
    int pos, temp;

    vga_set_auto_swap(0);
    vga_clear();
    
    vga_draw_box_double(15, 5, 50, 15, VGA_RED, VGA_BLCK);
    vga_fill_rect(16, 6, 48, 13, ' ', VGA_LRED, VGA_DGREY);

    vga_set_color(VGA_LRED, VGA_DGREY);
    const char* game_over = "GAME OVER!";
    int len = 0;
    while (game_over[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 8, game_over[i]);
    }

    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_putchr_at(38, 10, ':');
    vga_putchr_at(39, 10, '(');

    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* final_label = "Final Score";
    len = 0;
    while (final_label[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 12, final_label[i]);
    }

    pos = 0;
    temp = score;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp > 0) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    vga_set_color(VGA_LGREEN, VGA_DGREY);
    len = pos;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, buf[i]);
    }

    if (score >= high_score && score > 0) {
        vga_set_color(VGA_LCYAN, VGA_DGREY);
        const char* new_high = "** New High Score! **";
        len = 0;
        while (new_high[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 15, new_high[i]);
        }
    }

    vga_set_color(VGA_WHITE, VGA_DGREY);
    const char* inst = "Press any key to continue";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 17, inst[i]);
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();

    wait_for_char();
    game_state = GAME_2048_MENU;
    needs_redraw = 1;
}


void game_2048_save_high_score(void) {
    typedef struct {
        int magic_number;     
        int high_score;
        int version;           
        int checksum;
    } Game2048SaveData;
    
    Game2048SaveData save_data;
    save_data.magic_number = 0x32303438;  
    save_data.high_score = high_score;
    save_data.version = 1;
    
    save_data.checksum = save_data.magic_number + 
                         save_data.high_score + 
                         save_data.version;
    
    fat16_write_file(GAME_2048_SAVE_FILE, 
                     (const char*)&save_data, 
                     sizeof(Game2048SaveData));
}

void game_2048_load_high_score(void) {
    if (!fat16_file_exists(GAME_2048_SAVE_FILE)) {
        high_score = 0;
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_2048_SAVE_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        high_score = 0;
        return;
    }
    
    typedef struct {
        int magic_number;
        int high_score;
        int version;
        int checksum;
    } Game2048SaveData;
    
    if (file_size < sizeof(Game2048SaveData)) {
        high_score = 0;
        return;
    }
    
    Game2048SaveData* save_data = (Game2048SaveData*)file_content;
    
    if (save_data->magic_number != 0x32303438) {
        high_score = 0;
        return;
    }
    
    int calculated_checksum = save_data->magic_number + 
                             save_data->high_score + 
                             save_data->version;
    
    if (calculated_checksum != save_data->checksum) {
        high_score = 0;
        return;
    }
    
    high_score = save_data->high_score;
}

void game_2048_game_run(void) {
    game_2048_init();
    rand_seed = pit_get_ticks();
    
    vga_set_auto_swap(1);
    
    while (1) {
        game_2048_handle_input();
        
        if (game_state == GAME_2048_MENU) {
            if (needs_redraw) {
                game_2048_draw_menu();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == GAME_2048_PLAYING) {
            if (needs_redraw) {
                vga_set_auto_swap(0);
                vga_clear();
                game_2048_draw_ui();
                game_2048_draw_grid();
                vga_set_auto_swap(1);
                vga_swap_buffers();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == GAME_2048_PAUSED) {
            if (needs_redraw) {
                game_2048_draw_pause_screen();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == GAME_2048_WON) {
            if (needs_redraw) {
                vga_set_auto_swap(0);
                vga_clear();
                game_2048_draw_ui();
                game_2048_draw_grid();
                vga_set_auto_swap(1);
                vga_swap_buffers();
                needs_redraw = 0;
            }
            game_2048_draw_win_screen();
            pit_delay_ms(50);
        } else if (game_state == GAME_2048_OVER) {
            break;
        }
    }
    
    if (score > 0) {
        game_2048_draw_game_over();
    }
}

void game_2048_game_cleanup(void) {
    vga_set_auto_swap(1);
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}