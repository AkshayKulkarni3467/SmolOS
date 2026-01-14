#include "sos_minesweeper.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"


static Cell board[BOARD_HEIGHT][BOARD_WIDTH];
static BoardConfig current_config;
static CursorPos cursor;
static MinesweeperGameState game_state;
static GameDifficulty current_difficulty;
static int menu_selection;


static int flags_placed;
static int cells_revealed;
static uint32_t game_start_time;
static uint32_t game_end_time;
static int first_click;


static uint32_t best_time_beginner;
static uint32_t best_time_intermediate;
static uint32_t best_time_expert;


static uint32_t mine_rand_seed = 54321;

uint32_t minesweeper_rand(void) {
    mine_rand_seed = (mine_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return mine_rand_seed;
}


void minesweeper_game_init(void) {
    game_state = GAME_MINESWEEP_MENU;
    current_difficulty = DIFF_BEGINNER;
    menu_selection = 0;
    
    best_time_beginner = 999;
    best_time_intermediate = 999;
    best_time_expert = 999;
    
    minesweeper_load_stats();
}


BoardConfig minesweeper_get_config(GameDifficulty diff) {
    BoardConfig config;
    
    switch (diff) {
        case DIFF_BEGINNER:
            config.width = 9;
            config.height = 9;
            config.mine_count = 10;
            break;
        case DIFF_INTERMEDIATE:
            config.width = 16;
            config.height = 16;
            config.mine_count = 40;
            break;
        case DIFF_EXPERT:
            config.width = 30;
            config.height = 16;
            config.mine_count = 99;
            break;
        default:
            config.width = 9;
            config.height = 9;
            config.mine_count = 10;
            break;
    }
    
    return config;
}

void minesweeper_set_difficulty(GameDifficulty diff) {
    current_difficulty = diff;
    current_config = minesweeper_get_config(diff);
}


void minesweeper_reset_game(void) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x].has_mine = 0;
            board[y][x].adjacent_mines = 0;
            board[y][x].state = CELL_HIDDEN;
        }
    }
    

    cursor.x = current_config.width / 2;
    cursor.y = current_config.height / 2;
    

    flags_placed = 0;
    cells_revealed = 0;
    first_click = 1;
    
    game_start_time = pit_get_seconds();
    game_state = GAME_MINESWEEP_PLAYING;
}

void minesweeper_generate_board(int first_x, int first_y) {

    int mines_placed = 0;
    
    while (mines_placed < current_config.mine_count) {
        int x = minesweeper_rand() % current_config.width;
        int y = minesweeper_rand() % current_config.height;
        

        int dx = x - first_x;
        int dy = y - first_y;
        if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
            continue;
        }
        
        if (!board[y][x].has_mine) {
            board[y][x].has_mine = 1;
            mines_placed++;
        }
    }
    
    for (int y = 0; y < current_config.height; y++) {
        for (int x = 0; x < current_config.width; x++) {
            if (!board[y][x].has_mine) {
                board[y][x].adjacent_mines = minesweeper_count_adjacent_mines(x, y);
            }
        }
    }
    
    first_click = 0;
}

int minesweeper_count_adjacent_mines(int x, int y) {
    int count = 0;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (nx >= 0 && nx < current_config.width &&
                ny >= 0 && ny < current_config.height) {
                if (board[ny][nx].has_mine) {
                    count++;
                }
            }
        }
    }
    
    return count;
}

void minesweeper_reveal_cell(int x, int y) {
    if (x < 0 || x >= current_config.width || 
        y < 0 || y >= current_config.height) {
        return;
    }
    
    if (board[y][x].state != CELL_HIDDEN) {
        return;
    }
    
    if (first_click) {
        minesweeper_generate_board(x, y);
    }
    
    if (board[y][x].has_mine) {
        board[y][x].state = CELL_REVEALED;
        game_end_time = pit_get_seconds();
        game_state = GAME_MINESWEEP_LOST;
        minesweeper_reveal_all_mines();
        return;
    }
    
    board[y][x].state = CELL_REVEALED;
    cells_revealed++;
    
    if (board[y][x].adjacent_mines == 0) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                minesweeper_reveal_cell(x + dx, y + dy);
            }
        }
    }
    
    if (minesweeper_check_win()) {
        game_end_time = pit_get_seconds();
        game_state = GAME_MINESWEEP_WON;
        minesweeper_save_stats();
    }
}

void minesweeper_reveal_adjacent(int x, int y) {
    if (x < 0 || x >= current_config.width || 
        y < 0 || y >= current_config.height) {
        return;
    }
    
    if (board[y][x].state != CELL_REVEALED) {
        return;
    }
    
    int flag_count = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (nx >= 0 && nx < current_config.width &&
                ny >= 0 && ny < current_config.height) {
                if (board[ny][nx].state == CELL_FLAGGED) {
                    flag_count++;
                }
            }
        }
    }
    
    if (flag_count == board[y][x].adjacent_mines) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                
                int nx = x + dx;
                int ny = y + dy;
                
                if (nx >= 0 && nx < current_config.width &&
                    ny >= 0 && ny < current_config.height) {
                    if (board[ny][nx].state == CELL_HIDDEN) {
                        minesweeper_reveal_cell(nx, ny);
                    }
                }
            }
        }
    }
}

void minesweeper_toggle_flag(int x, int y) {
    if (x < 0 || x >= current_config.width || 
        y < 0 || y >= current_config.height) {
        return;
    }
    
    if (board[y][x].state == CELL_HIDDEN) {
        board[y][x].state = CELL_FLAGGED;
        flags_placed++;
    } else if (board[y][x].state == CELL_FLAGGED) {
        board[y][x].state = CELL_HIDDEN;
        flags_placed--;
    }
}

void minesweeper_toggle_question(int x, int y) {
    if (x < 0 || x >= current_config.width || 
        y < 0 || y >= current_config.height) {
        return;
    }
    
    if (board[y][x].state == CELL_HIDDEN) {
        board[y][x].state = CELL_QUESTION;
    } else if (board[y][x].state == CELL_QUESTION) {
        board[y][x].state = CELL_HIDDEN;
    }
}

void minesweeper_reveal_all_mines(void) {
    for (int y = 0; y < current_config.height; y++) {
        for (int x = 0; x < current_config.width; x++) {
            if (board[y][x].has_mine) {
                board[y][x].state = CELL_REVEALED;
            }
        }
    }
}

int minesweeper_check_win(void) {
    int total_safe_cells = current_config.width * current_config.height - current_config.mine_count;
    return (cells_revealed == total_safe_cells);
}


void minesweeper_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == GAME_MINESWEEP_MENU) {
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 4;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 4) menu_selection = 0;
        } else if (c == '\n') { 
            if (menu_selection == 0) {
                minesweeper_set_difficulty(current_difficulty);
                minesweeper_reset_game();
            } else if (menu_selection >= 1 && menu_selection <= 3) {
                current_difficulty = menu_selection - 1;
            } else if (menu_selection == 4) {
                game_state = GAME_MINESWEEP_QUIT;
            }
        } else if (c == 27) { 
            game_state = GAME_MINESWEEP_QUIT;
        }
    } else if (game_state == GAME_MINESWEEP_PLAYING) {
        if (c == 0x11) {  
            if (cursor.y > 0) cursor.y--;
        } else if (c == 0x12) {  
            if (cursor.y < current_config.height - 1) cursor.y++;
        } else if (c == 0x13) {  
            if (cursor.x > 0) cursor.x--;
        } else if (c == 0x14) { 
            if (cursor.x < current_config.width - 1) cursor.x++;
        } else if (c == ' ' || c == '\n') {  
            minesweeper_reveal_cell(cursor.x, cursor.y);
        } else if (c == 'f' || c == 'F') {  
            minesweeper_toggle_flag(cursor.x, cursor.y);
        } else if (c == 'q' || c == 'Q') {  
            minesweeper_toggle_question(cursor.x, cursor.y);
        } else if (c == 'd' || c == 'D') {  
            minesweeper_reveal_adjacent(cursor.x, cursor.y);
        } else if (c == 'p' || c == 'P') {  
            game_state = GAME_MINESWEEP_PAUSED;
        } else if (c == 27) {  
            game_state = GAME_MINESWEEP_MENU;
        }
    } else if (game_state == GAME_MINESWEEP_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = GAME_MINESWEEP_PLAYING;
        } else if (c == 27) {
            game_state = GAME_MINESWEEP_MENU;
        }
    } else if (game_state == GAME_MINESWEEP_WON || game_state == GAME_MINESWEEP_LOST) {
        if (c == '\n' || c == ' ') {
            game_state = GAME_MINESWEEP_MENU;
        } else if (c == 'r' || c == 'R') {
            minesweeper_reset_game();
        }
    }
}


void minesweeper_draw_cell(int x, int y) {
    int screen_x = BOARD_OFFSET_X + x * 2;
    int screen_y = BOARD_OFFSET_Y + y;
    
    Cell* cell = &board[y][x];
    
    int is_cursor = (cursor.x == x && cursor.y == y);
    
    uint8_t bg_color = is_cursor ? VGA_DGREY : VGA_BLCK;
    
    if (cell->state == CELL_HIDDEN) {
        vga_set_color(VGA_LGREY, bg_color);
        vga_putchr_at(screen_x, screen_y, 0xB0);  
        vga_putchr_at(screen_x + 1, screen_y, 0xB0);
    } else if (cell->state == CELL_FLAGGED) {
        vga_set_color(VGA_LRED, bg_color);
        vga_putchr_at(screen_x, screen_y, 0x10);  
        vga_putchr_at(screen_x + 1, screen_y, ' ');
    } else if (cell->state == CELL_QUESTION) {
        vga_set_color(VGA_YELLOW, bg_color);
        vga_putchr_at(screen_x, screen_y, '?');
        vga_putchr_at(screen_x + 1, screen_y, ' ');
    } else if (cell->state == CELL_REVEALED) {
        if (cell->has_mine) {
            vga_set_color(VGA_LRED, bg_color);
            vga_putchr_at(screen_x, screen_y, '*');
            vga_putchr_at(screen_x + 1, screen_y, ' ');
        } else {
            uint8_t color;
            switch (cell->adjacent_mines) {
                case 0:  color = VGA_DGREY; break;
                case 1:  color = VGA_BLUE; break;
                case 2:  color = VGA_GREEN; break;
                case 3:  color = VGA_RED; break;
                case 4:  color = VGA_BLUE; break;
                case 5:  color = VGA_BRWN; break;
                case 6:  color = VGA_CYAN; break;
                case 7:  color = VGA_BLCK; break;
                case 8:  color = VGA_DGREY; break;
                default: color = VGA_WHITE; break;
            }
            
            vga_set_color(color, bg_color);
            if (cell->adjacent_mines == 0) {
                vga_putchr_at(screen_x, screen_y, ' ');
            } else {
                vga_putchr_at(screen_x, screen_y, '0' + cell->adjacent_mines);
            }
            vga_putchr_at(screen_x + 1, screen_y, ' ');
        }
    }
}

void minesweeper_draw_board(void) {
    int border_width = current_config.width * 2 + 2;
    int border_height = current_config.height + 2;
    
    vga_draw_box_double(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1, 
                        border_width, border_height, VGA_CYAN, VGA_BLCK);
    
    for (int y = 0; y < current_config.height; y++) {
        for (int x = 0; x < current_config.width; x++) {
            minesweeper_draw_cell(x, y);
        }
    }
}

void minesweeper_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== MINESWEEPER ===", 0);
    
    int info_y = BOARD_OFFSET_Y + current_config.height + 2;
    
    vga_draw_box_single(0, info_y, 80, 3, VGA_YELLOW, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    int mines_remaining = current_config.mine_count - flags_placed;
    
    char mines_str[20];
    int pos = 0;
    const char* prefix = "Mines: ";
    for (int i = 0; prefix[i]; i++) mines_str[pos++] = prefix[i];
    
    if (mines_remaining < 0) {
        mines_str[pos++] = '-';
        mines_remaining = -mines_remaining;
    }
    
    int temp = mines_remaining;
    if (temp == 0) {
        mines_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            mines_str[pos++] = digits[i];
        }
    }
    mines_str[pos] = '\0';
    
    for (int i = 0; mines_str[i]; i++) {
        vga_putchr_at(2 + i, info_y + 1, mines_str[i]);
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    uint32_t elapsed = pit_get_seconds() - game_start_time;
    
    char time_str[20];
    pos = 0;
    const char* time_prefix = "Time: ";
    for (int i = 0; time_prefix[i]; i++) time_str[pos++] = time_prefix[i];
    
    temp = elapsed;
    if (temp == 0) {
        time_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            time_str[pos++] = digits[i];
        }
    }
    time_str[pos++] = 's';
    time_str[pos] = '\0';
    
    for (int i = 0; time_str[i]; i++) {
        vga_putchr_at(20 + i, info_y + 1, time_str[i]);
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls1 = "Arrows:Move  Space:Reveal  F:Flag  Q:Mark  D:Chord";
    const char* controls2 = "P:Pause  ESC:Menu";
    
    for (int i = 0; controls1[i]; i++) {
        vga_putchr_at(2 + i, info_y + 2, controls1[i]);
    }
    for (int i = 0; controls2[i]; i++) {
        vga_putchr_at(55 + i, info_y + 2, controls2[i]);
    }
}

void minesweeper_draw_menu(void) {
    vga_clear();
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* title[] = {
        " __  __ ___ _   _ _____ ______        _______ _____ ____  _____ ____  ",
        "|  \\/  |_ _| \\ | | ____/ ___\\ \\      / / ____| ____|  _ \\| ____|  _ \\ ",
        "| |\\/| || ||  \\| |  _| \\___ \\\\ \\ /\\ / /|  _| |  _| | |_) |  _| | |_) |",
        "| |  | || || |\\  | |___ ___) |\\ V  V / | |___| |___|  __/| |___|  _ < ",
        "|_|  |_|___|_| \\_|_____|____/  \\_/\\_/  |_____|_____|_|   |_____|_| \\_\\"
    };
    
    uint8_t colors[] = {VGA_LRED, VGA_RED, VGA_YELLOW, VGA_LRED, VGA_RED};
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        vga_set_color(colors[i], VGA_BLCK);
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 2 + i, title[i][j]);
        }
    }
    
    const char* options[] = {
        "Play Game",
        "Beginner (9x9, 10 mines)",
        "Intermediate (16x16, 40 mines)",
        "Expert (30x16, 99 mines)",
        "Exit"
    };
    
    for (int i = 0; i < 5; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_fill_rect(15, 10 + i * 2, 50, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 17;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 10 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 10 + i * 2, options[i][j]);
        }
        
        if (i >= 1 && i <= 3 && current_difficulty == i - 1) {
            vga_set_color(VGA_LGREEN, selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(x + 2, 10 + i * 2, '<');
        }
    }
    
    vga_draw_box_single(15, 21, 50, 3, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* best_title = "Best Times";
    int len = 0;
    while (best_title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 21, best_title[i]);
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    char best_str[60];
    
    int pos = 0;
    const char* beg = "Beginner: ";
    for (int i = 0; beg[i]; i++) best_str[pos++] = beg[i];
    if (best_time_beginner < 999) {
        int temp = best_time_beginner;
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            best_str[pos++] = digits[i];
        }
        best_str[pos++] = 's';
    } else {
        const char* none = "---";
        for (int i = 0; none[i]; i++) best_str[pos++] = none[i];
    }
    best_str[pos] = '\0';
    for (int i = 0; best_str[i]; i++) {
        vga_putchr_at(17 + i, 22, best_str[i]);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN, ENTER to select, ESC to exit";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 24, inst[i]);
    }
}

void minesweeper_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    minesweeper_draw_ui();
    minesweeper_draw_board();
    
    vga_end_batch();
}

void minesweeper_draw_pause_screen(void) {
    vga_draw_box_double(25, 10, 30, 5, VGA_YELLOW, VGA_BLCK);
    vga_fill_rect(26, 11, 28, 3, ' ', VGA_YELLOW, VGA_DGREY);
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* pause = "PAUSED";
    int len = 0;
    while (pause[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 11, pause[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_DGREY);
    const char* inst = "Press P to continue";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, inst[i]);
    }
}

void minesweeper_draw_win_screen(void) {
    vga_clear();
    
    vga_draw_box_double(10, 5, 60, 15, VGA_LGREEN, VGA_BLCK);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* win = "VICTORY!";
    int len = 0;
    while (win[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 7, win[i]);
    }
    
    uint32_t time_taken = game_end_time - game_start_time;
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char time_str[40];
    int pos = 0;
    const char* time_prefix = "Time: ";
    for (int i = 0; time_prefix[i]; i++) time_str[pos++] = time_prefix[i];
    
    int temp = time_taken;
    if (temp == 0) {
        time_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            time_str[pos++] = digits[i];
        }
    }
    time_str[pos++] = ' ';
    time_str[pos++] = 's';
    time_str[pos++] = 'e';
    time_str[pos++] = 'c';
    time_str[pos++] = 'o';
    time_str[pos++] = 'n';
    time_str[pos++] = 'd';
    time_str[pos++] = 's';
    time_str[pos] = '\0';
    
    len = 0;
    while (time_str[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, time_str[i]);
    }
    
    int is_record = 0;
    if (current_difficulty == DIFF_BEGINNER && time_taken < best_time_beginner) {
        is_record = 1;
    } else if (current_difficulty == DIFF_INTERMEDIATE && time_taken < best_time_intermediate) {
        is_record = 1;
    } else if (current_difficulty == DIFF_EXPERT && time_taken < best_time_expert) {
        is_record = 1;
    }
    
    if (is_record) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        const char* record = "NEW BEST TIME!";
        len = 0;
        while (record[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 12, record[i]);
        }
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst1 = "ENTER - Return to menu";
    const char* inst2 = "R - Play again";
    
    len = 0;
    while (inst1[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, inst1[i]);
    }
    
    len = 0;
    while (inst2[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 17, inst2[i]);
    }
}

void minesweeper_draw_lose_screen(void) {
    vga_clear();
    
    vga_draw_box_double(10, 5, 60, 15, VGA_RED, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* lose = "GAME OVER!";
    int len = 0;
    while (lose[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 7, lose[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* msg = "You hit a mine!";
    len = 0;
    while (msg[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, msg[i]);
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    char stats[40];
    int pos = 0;
    const char* cells = "Cells revealed: ";
    for (int i = 0; cells[i]; i++) stats[pos++] = cells[i];
    
    int temp = cells_revealed;
    if (temp == 0) {
        stats[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stats[pos++] = digits[i];
        }
    }
    stats[pos] = '\0';
    
    len = 0;
    while (stats[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 12, stats[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst1 = "ENTER - Return to menu";
    const char* inst2 = "R - Try again";
    
    len = 0;
    while (inst1[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, inst1[i]);
    }
    
    len = 0;
    while (inst2[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 17, inst2[i]);
    }
}


void minesweeper_save_stats(void) {
    uint32_t time_taken = game_end_time - game_start_time;
    
    if (current_difficulty == DIFF_BEGINNER && time_taken < best_time_beginner) {
        best_time_beginner = time_taken;
    } else if (current_difficulty == DIFF_INTERMEDIATE && time_taken < best_time_intermediate) {
        best_time_intermediate = time_taken;
    } else if (current_difficulty == DIFF_EXPERT && time_taken < best_time_expert) {
        best_time_expert = time_taken;
    }
}

void minesweeper_load_stats(void) {
    //TODO Implement saving scores to FAT16
}

void minesweeper_format_time(uint32_t seconds, char* buffer) {
    int pos = 0;
    
    if (seconds >= 60) {
        int minutes = seconds / 60;
        seconds %= 60;
        
        if (minutes >= 10) buffer[pos++] = '0' + (minutes / 10);
        buffer[pos++] = '0' + (minutes % 10);
        buffer[pos++] = ':';
    }
    
    if (seconds >= 10) buffer[pos++] = '0' + (seconds / 10);
    buffer[pos++] = '0' + (seconds % 10);
    buffer[pos++] = 's';
    buffer[pos] = '\0';
}


void minesweeper_game_run(void) {
    minesweeper_game_init();
    
    while (game_state != GAME_MINESWEEP_QUIT) {
        minesweeper_handle_input();
        
        if (game_state == GAME_MINESWEEP_MENU) {
            minesweeper_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == GAME_MINESWEEP_PLAYING) {
            minesweeper_draw_game();
            pit_delay_ms(50);
        } else if (game_state == GAME_MINESWEEP_PAUSED) {
            minesweeper_draw_pause_screen();
            pit_delay_ms(50);
        } else if (game_state == GAME_MINESWEEP_WON) {
            minesweeper_draw_win_screen();
            pit_delay_ms(50);
        } else if (game_state == GAME_MINESWEEP_LOST) {
            minesweeper_draw_lose_screen();
            pit_delay_ms(50);
        }
    }
}

void minesweeper_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}