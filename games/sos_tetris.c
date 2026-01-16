#include "sos_tetris.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_TETRIS_SAVE_FILE "TESTRISSAVE.DAT"

static uint8_t board[BOARD_HEIGHT][BOARD_WIDTH];
static Piece current_piece;
static Piece next_pieces[3];
static Piece held_piece;
static int can_hold;

static TetrisState game_state;
static int score;
static int high_score;
static int level;
static int lines_cleared;
static int combo;
static uint32_t drop_speed;
static uint32_t last_drop_time;
static int menu_selection;

static uint32_t tetris_rand_seed = 54321;

static uint32_t tetris_rand(void) {
    tetris_rand_seed = (tetris_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return tetris_rand_seed;
}


void tetris_get_piece_blocks(PieceType type, int rotation, int blocks[4][2]) {
    rotation = rotation % 4;
    
    switch (type) {
        case PIECE_I:
            if (rotation == 0 || rotation == 2) {
                blocks[0][0] = 0; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 0;
                blocks[2][0] = 2; blocks[2][1] = 0;
                blocks[3][0] = 3; blocks[3][1] = 0;
            } else {
                blocks[0][0] = 1; blocks[0][1] = -1;
                blocks[1][0] = 1; blocks[1][1] = 0;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            }
            break;
            
        case PIECE_O:
            blocks[0][0] = 0; blocks[0][1] = 0;
            blocks[1][0] = 1; blocks[1][1] = 0;
            blocks[2][0] = 0; blocks[2][1] = 1;
            blocks[3][0] = 1; blocks[3][1] = 1;
            break;
            
        case PIECE_T:
            if (rotation == 0) {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 0; blocks[1][1] = 1;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 2; blocks[3][1] = 1;
            } else if (rotation == 1) {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 2; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            } else if (rotation == 2) {
                blocks[0][0] = 0; blocks[0][1] = 1;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 2; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            } else {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 0; blocks[1][1] = 1;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            }
            break;
            
        case PIECE_S:
            if (rotation == 0 || rotation == 2) {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 2; blocks[1][1] = 0;
                blocks[2][0] = 0; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 1;
            } else {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 2; blocks[2][1] = 1;
                blocks[3][0] = 2; blocks[3][1] = 2;
            }
            break;
            
        case PIECE_Z:
            if (rotation == 0 || rotation == 2) {
                blocks[0][0] = 0; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 0;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 2; blocks[3][1] = 1;
            } else {
                blocks[0][0] = 2; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 2; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            }
            break;
            
        case PIECE_J:
            if (rotation == 0) {
                blocks[0][0] = 0; blocks[0][1] = 0;
                blocks[1][0] = 0; blocks[1][1] = 1;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 2; blocks[3][1] = 1;
            } else if (rotation == 1) {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 2; blocks[1][1] = 0;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            } else if (rotation == 2) {
                blocks[0][0] = 0; blocks[0][1] = 1;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 2; blocks[2][1] = 1;
                blocks[3][0] = 2; blocks[3][1] = 2;
            } else {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 0; blocks[2][1] = 2;
                blocks[3][0] = 1; blocks[3][1] = 2;
            }
            break;
            
        case PIECE_L:
            if (rotation == 0) {
                blocks[0][0] = 2; blocks[0][1] = 0;
                blocks[1][0] = 0; blocks[1][1] = 1;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 2; blocks[3][1] = 1;
            } else if (rotation == 1) {
                blocks[0][0] = 1; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 1; blocks[2][1] = 2;
                blocks[3][0] = 2; blocks[3][1] = 2;
            } else if (rotation == 2) {
                blocks[0][0] = 0; blocks[0][1] = 1;
                blocks[1][0] = 1; blocks[1][1] = 1;
                blocks[2][0] = 2; blocks[2][1] = 1;
                blocks[3][0] = 0; blocks[3][1] = 2;
            } else {
                blocks[0][0] = 0; blocks[0][1] = 0;
                blocks[1][0] = 1; blocks[1][1] = 0;
                blocks[2][0] = 1; blocks[2][1] = 1;
                blocks[3][0] = 1; blocks[3][1] = 2;
            }
            break;
            
        default:
            break;
    }
}

uint8_t tetris_get_piece_color(PieceType type) {
    switch (type) {
        case PIECE_I: return VGA_CYAN;
        case PIECE_O: return VGA_YELLOW;
        case PIECE_T: return VGA_MAGENTA;
        case PIECE_S: return VGA_LGREEN;
        case PIECE_Z: return VGA_LRED;
        case PIECE_J: return VGA_LBLUE;
        case PIECE_L: return VGA_LRED;
        default: return VGA_WHITE;
    }
}


void tetris_game_init(void) {
    game_state = TETRIS_MENU;
    menu_selection = 0;
    high_score = 0;
    tetris_load_high_score();
}

static void tetris_reset_game(void) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            board[y][x] = 0;
        }
    }
    
    score = 0;
    level = 1;
    lines_cleared = 0;
    combo = 0;
    drop_speed = 500;
    last_drop_time = pit_get_total_milliseconds();
    can_hold = 1;
    
    held_piece.type = PIECE_NONE;
    
    for (int i = 0; i < 3; i++) {
        next_pieces[i].type = tetris_rand() % 7;
        next_pieces[i].color = tetris_get_piece_color(next_pieces[i].type);
    }
    
    tetris_spawn_piece();
}

void tetris_spawn_piece(void) {
    current_piece = next_pieces[0];
    current_piece.x = BOARD_WIDTH / 2 - 1;
    current_piece.y = 0;
    current_piece.rotation = 0;
    
    next_pieces[0] = next_pieces[1];
    next_pieces[1] = next_pieces[2];
    next_pieces[2].type = tetris_rand() % 7;
    next_pieces[2].color = tetris_get_piece_color(next_pieces[2].type);
    
    can_hold = 1;
    
    last_drop_time = pit_get_total_milliseconds();
    
    if (tetris_check_collision(&current_piece, 0, 0)) {
        game_state = TETRIS_GAME_OVER;
    }
}

int tetris_check_collision(Piece* piece, int dx, int dy) {
    int blocks[4][2];
    tetris_get_piece_blocks(piece->type, piece->rotation, blocks);
    
    for (int i = 0; i < 4; i++) {
        int x = piece->x + blocks[i][0] + dx;
        int y = piece->y + blocks[i][1] + dy;
        
        if (x < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT) {
            return 1;
        }
        
        if (y >= 0 && board[y][x] != 0) {
            return 1;
        }
    }
    
    return 0;
}

void tetris_move_piece(int dx, int dy) {
    if (!tetris_check_collision(&current_piece, dx, dy)) {
        current_piece.x += dx;
        current_piece.y += dy;
    } else if (dy > 0) {
        tetris_lock_piece();
    }
}

void tetris_rotate_piece(void) {
    int old_rotation = current_piece.rotation;
    current_piece.rotation = (current_piece.rotation + 1) % 4;
    
    if (tetris_check_collision(&current_piece, 0, 0)) {
        if (!tetris_check_collision(&current_piece, -1, 0)) {
            current_piece.x--;
        } else if (!tetris_check_collision(&current_piece, 1, 0)) {
            current_piece.x++;
        } else if (!tetris_check_collision(&current_piece, 0, -1)) {
            current_piece.y--;
        } else {
            current_piece.rotation = old_rotation;
        }
    }
}

void tetris_hard_drop(void) {
    while (!tetris_check_collision(&current_piece, 0, 1)) {
        current_piece.y++;
        score += 2;  
    }
    tetris_lock_piece();
}

void tetris_lock_piece(void) {
    int blocks[4][2];
    tetris_get_piece_blocks(current_piece.type, current_piece.rotation, blocks);
    
    for (int i = 0; i < 4; i++) {
        int x = current_piece.x + blocks[i][0];
        int y = current_piece.y + blocks[i][1];
        
        if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH) {
            board[y][x] = current_piece.color;
        }
    }
    
    tetris_clear_lines();
    tetris_spawn_piece();
}

void tetris_clear_lines(void) {
    int cleared = 0;
    
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (board[y][x] == 0) {
                full = 0;
                break;
            }
        }
        
        if (full) {
            cleared++;
            
            for (int yy = y; yy > 0; yy--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    board[yy][x] = board[yy - 1][x];
                }
            }
            
            for (int x = 0; x < BOARD_WIDTH; x++) {
                board[0][x] = 0;
            }
            
            y++;  
        }
    }
    
    if (cleared > 0) {
        lines_cleared += cleared;
        combo++;
        score += tetris_calculate_score(cleared, level) * combo;
        
        level = (lines_cleared / 10) + 1;
        drop_speed = 1000 - (level * 50);
        if (drop_speed < 100) drop_speed = 100;
    } else {
        combo = 0;
    }
}

int tetris_calculate_score(int lines, int lvl) {
    switch (lines) {
        case 1: return 100 * lvl;
        case 2: return 300 * lvl;
        case 3: return 500 * lvl;
        case 4: return 800 * lvl;  
        default: return 0;
    }
}

void tetris_update_game(void) {
    uint32_t current_time = pit_get_total_milliseconds();
    
    uint32_t time_diff;
    if (current_time >= last_drop_time) {
        time_diff = current_time - last_drop_time;
    } else {
        time_diff = (0xFFFFFFFF - last_drop_time) + current_time + 1;
    }
    
    if (time_diff >= drop_speed) {
        tetris_move_piece(0, 1);
        last_drop_time = current_time;
    }
}


void tetris_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == TETRIS_MENU) {
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 1;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 1) menu_selection = 0;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                tetris_reset_game();
                game_state = TETRIS_PLAYING;
            } else if (menu_selection == 1) {
                game_state = TETRIS_EXIT;
            }
        } else if (c == 27) {  
            game_state = TETRIS_EXIT;
        }
    }else if (game_state == TETRIS_PLAYING) {
        if (c == 0x13) {  
            tetris_move_piece(-1, 0);
        } else if (c == 0x14) {  
            tetris_move_piece(1, 0);
        } else if (c == 0x12) {  
            tetris_move_piece(0, 1);
            score += 1;
        } else if (c == 0x11 || c == 'w' || c == 'W') {  
            tetris_rotate_piece();
        } else if (c == ' ') {  
            tetris_hard_drop();
        } else if (c == 'c' || c == 'C') {  
            if (can_hold) {
                Piece temp = current_piece;
                if (held_piece.type == PIECE_NONE) {
                    tetris_spawn_piece();
                } else {
                    current_piece = held_piece;
                    current_piece.x = BOARD_WIDTH / 2 - 1;
                    current_piece.y = 0;
                    current_piece.rotation = 0;
                }
                held_piece = temp;
                can_hold = 0;
            }
        } else if (c == 'p' || c == 'P') {
            game_state = TETRIS_PAUSED;
        } else if (c == 27) {  
            game_state = TETRIS_MENU;
        }
    } else if (game_state == TETRIS_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = TETRIS_PLAYING;
        } else if (c == 27) {  
            game_state = TETRIS_MENU;
        }
    }
}


void tetris_draw_board(void) {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int screen_x = BOARD_OFFSET_X + x * 2;
            int screen_y = BOARD_OFFSET_Y + y;
            
            if (board[y][x] == 0) {
                uint8_t color = ((x + y) % 2 == 0) ? VGA_DGREY : VGA_BLCK;
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 0xB0);
                vga_putchr_at(screen_x + 1, screen_y, 0xB0);
            } else {
                vga_set_color(board[y][x], VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 0xDB);
                vga_putchr_at(screen_x + 1, screen_y, 0xDB);
            }
        }
    }
    
    vga_draw_box_double(BOARD_OFFSET_X - 1, BOARD_OFFSET_Y - 1, 
                        BOARD_WIDTH * 2 + 2, BOARD_HEIGHT + 2, VGA_CYAN, VGA_BLCK);
}

void tetris_draw_ghost_piece(void) {
    Piece ghost = current_piece;
    while (!tetris_check_collision(&ghost, 0, 1)) {
        ghost.y++;
    }
    
    int blocks[4][2];
    tetris_get_piece_blocks(ghost.type, ghost.rotation, blocks);
    
    for (int i = 0; i < 4; i++) {
        int x = ghost.x + blocks[i][0];
        int y = ghost.y + blocks[i][1];
        
        if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH) {
            int screen_x = BOARD_OFFSET_X + x * 2;
            int screen_y = BOARD_OFFSET_Y + y;
            
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_putchr_at(screen_x, screen_y, '[');
            vga_putchr_at(screen_x + 1, screen_y, ']');
        }
    }
}

void tetris_draw_piece(Piece* piece) {
    int blocks[4][2];
    tetris_get_piece_blocks(piece->type, piece->rotation, blocks);
    
    for (int i = 0; i < 4; i++) {
        int x = piece->x + blocks[i][0];
        int y = piece->y + blocks[i][1];
        
        if (y >= 0 && y < BOARD_HEIGHT && x >= 0 && x < BOARD_WIDTH) {
            int screen_x = BOARD_OFFSET_X + x * 2;
            int screen_y = BOARD_OFFSET_Y + y;
            
            vga_set_color(piece->color, VGA_BLCK);
            vga_putchr_at(screen_x, screen_y, 0xDB);
            vga_putchr_at(screen_x + 1, screen_y, 0xDB);
        }
    }
}

void tetris_draw_next_pieces(void) {
    vga_draw_box_single(52, 2, 18, 12, VGA_YELLOW, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    const char* title = "NEXT";
    for (int i = 0; title[i]; i++) {
        vga_putchr_at(58 + i, 3, title[i]);
    }
    
    for (int p = 0; p < 3; p++) {
        Piece preview = next_pieces[p];
        preview.x = 0;
        preview.y = 0;
        preview.rotation = 0;
        
        int blocks[4][2];
        tetris_get_piece_blocks(preview.type, preview.rotation, blocks);
        
        int base_x = 57;
        int base_y = 5 + p * 3;
        
        for (int i = 0; i < 4; i++) {
            int x = base_x + blocks[i][0];
            int y = base_y + blocks[i][1];
            
            vga_set_color(preview.color, VGA_BLCK);
            vga_putchr_at(x, y, 0xDB);
        }
    }
}

void tetris_draw_held_piece(void) {
    vga_draw_box_single(9, 2, 18, 6, VGA_MAGENTA, VGA_BLCK);
    vga_set_color(VGA_MAGENTA, VGA_BLCK);
    
    const char* title = "HOLD (C)";
    for (int i = 0; title[i]; i++) {
        vga_putchr_at(13 + i, 3, title[i]);
    }
    
    if (held_piece.type != PIECE_NONE) {
        Piece preview = held_piece;
        preview.x = 0;
        preview.y = 0;
        preview.rotation = 0;
        
        int blocks[4][2];
        tetris_get_piece_blocks(preview.type, preview.rotation, blocks);
        
        int base_x = 15;
        int base_y = 5;
        
        uint8_t color = can_hold ? preview.color : VGA_DGREY;
        
        for (int i = 0; i < 4; i++) {
            int x = base_x + blocks[i][0];
            int y = base_y + blocks[i][1];
            
            vga_set_color(color, VGA_BLCK);
            vga_putchr_at(x, y, 0xDB);
        }
    }
}

void tetris_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== TETRIS ===", 0);
    
    vga_draw_box_single(9, 9, 18, 8, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    const char* labels[] = {"Score:", "Level:", "Lines:", "Combo:"};
    int values[] = {score, level, lines_cleared, combo};
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; labels[i][j]; j++) {
            vga_putchr_at(11 + j, 10 + i * 2, labels[i][j]);
        }
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        char num[12];
        int pos = 0;
        int temp = values[i];
        if (temp == 0) {
            num[pos++] = '0';
        } else {
            char digits[10];
            int digit_count = 0;
            while (temp > 0) {
                digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int k = digit_count - 1; k >= 0; k--) {
                num[pos++] = digits[k];
            }
        }
        num[pos] = '\0';
        
        for (int j = 0; num[j]; j++) {
            vga_putchr_at(18 + j, 10 + i * 2, num[j]);
        }
        
        vga_set_color(VGA_YELLOW, VGA_BLCK);
    }
    
    vga_draw_box_single(9, 18, 18, 5, VGA_GREEN, VGA_BLCK);
    vga_set_color(VGA_LGREY, VGA_BLCK);
    
    const char* controls[] = {
        "Arrows:Move",
        "Up:Rotate",
        "Space:Drop",
        "C:Hold"
    };
    
    for (int i = 0; i < 4; i++) {
        int x = 10;
        for (int j = 0; controls[i][j]; j++) {
            vga_putchr_at(x++, 19 + i, controls[i][j]);
        }
    }
    
    vga_draw_box_single(52, 15, 18, 8, VGA_LRED, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    const char* stats_title = "HIGH SCORE";
    for (int i = 0; stats_title[i]; i++) {
        vga_putchr_at(55 + i, 16, stats_title[i]);
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char hs_num[12];
    int pos = 0;
    int temp = high_score;
    if (temp == 0) {
        hs_num[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int k = digit_count - 1; k >= 0; k--) {
            hs_num[pos++] = digits[k];
        }
    }
    hs_num[pos] = '\0';
    
    for (int i = 0; hs_num[i]; i++) {
        vga_putchr_at(57 + i, 18, hs_num[i]);
    }
}

void tetris_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    tetris_draw_ui();
    tetris_draw_board();
    tetris_draw_ghost_piece();
    tetris_draw_piece(&current_piece);
    tetris_draw_next_pieces();
    tetris_draw_held_piece();
    
    vga_end_batch();
}

void tetris_draw_menu(void) {
    vga_clear();
    
    const char* title[] = {
        " _____ _____ _____ ____  ___ _____ ",
        "|_   _| ____|_   _|  _ \\|_ _/ ____|",
        "  | | |  _|   | | | |_) | |\\___ \\ ",
        "  | | | |___  | | |  _ <| | ___) |",
        "  |_| |_____| |_| |_| \\_\\___|____/ "
    };
    
    uint8_t rainbow_colors[] = {VGA_LRED, VGA_YELLOW, VGA_LGREEN, VGA_CYAN, VGA_LBLUE};
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        vga_set_color(rainbow_colors[i], VGA_BLCK);
        
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 4 + i, title[i][j]);
        }
    }
    
    const char* options[] = {
        "Play Game",
        "Exit"
    };
    
    for (int i = 0; i < 2; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(30, 12 + i * 2, 20, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 32;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, options[i][j]);
        }
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char hs[30];
    int pos = 0;
    const char* hs_text = "High Score: ";
    for (int i = 0; hs_text[i]; i++) hs[pos++] = hs_text[i];
    
    int temp = high_score;
    if (temp == 0) {
        hs[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            hs[pos++] = digits[i];
        }
    }
    hs[pos] = '\0';
    
    int len = 0;
    while (hs[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 10, hs[i]);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* controls[] = {
        "Controls:",
        "Arrows - Move piece",
        "Up/W - Rotate",
        "Space - Hard drop",
        "C - Hold piece",
        "P - Pause"
    };
    
    for (int i = 0; i < 6; i++) {
        len = 0;
        while (controls[i][len]) len++;
        int x = (80 - len) / 2;
        
        uint8_t color = (i == 0) ? VGA_LCYAN : VGA_LGREY;
        vga_set_color(color, VGA_BLCK);
        
        for (int j = 0; controls[i][j]; j++) {
            vga_putchr_at(x + j, 17 + i, controls[i][j]);
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to exit";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 24, inst[i]);
    }
}

void tetris_save_high_score(void) {
    typedef struct {
        int magic_number;     
        int high_score;
        int version;           
        int checksum;
    } GameTetrisSaveData;
    
    GameTetrisSaveData save_data;
    save_data.magic_number = 0x23232323;  
    save_data.high_score = high_score;
    save_data.version = 1;
    
    save_data.checksum = save_data.magic_number + 
                         save_data.high_score + 
                         save_data.version;
    
    fat16_write_file(GAME_TETRIS_SAVE_FILE, 
                     (const char*)&save_data, 
                     sizeof(GameTetrisSaveData));}

void tetris_load_high_score(void) {
    if (!fat16_file_exists(GAME_TETRIS_SAVE_FILE)) {
        high_score = 0;
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_TETRIS_SAVE_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        high_score = 0;
        return;
    }
    
    typedef struct {
        int magic_number;
        int high_score;
        int version;
        int checksum;
    } GameTetrisSaveData;
    
    if (file_size < sizeof(GameTetrisSaveData)) {
        high_score = 0;
        return;
    }
    
    GameTetrisSaveData* save_data = (GameTetrisSaveData*)file_content;
    
    if (save_data->magic_number != 0x23232323) {
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

void tetris_game_run(void) {
    tetris_game_init();
    
    while (1) {
        tetris_handle_input();
        
        if (game_state == TETRIS_EXIT) {
            break;  
        }
        if (game_state == TETRIS_MENU) {
            tetris_draw_menu();
            pit_delay_ms(50);
            
            if (menu_selection == 1) {
                keyboard_poll();
                if (has_key()) {
                    char c = get_char();
                    if (c == '\n') {
                        break;  
                    }
                }
            }
        } else if (game_state == TETRIS_PLAYING) {
            tetris_update_game(); 
            tetris_draw_game();
            pit_delay_ms(16);
        } else if (game_state == TETRIS_PAUSED) {
            tetris_draw_game();  
            tetris_draw_pause_screen();  
            pit_delay_ms(50);
        } else if (game_state == TETRIS_GAME_OVER) {
            tetris_draw_game_over();  
            game_state = TETRIS_MENU;
        }
    }
}

void tetris_draw_pause_screen(void) {
    vga_draw_box_double(25, 10, 30, 5, VGA_YELLOW, VGA_BLCK);
    vga_fill_rect(26, 11, 28, 3, ' ', VGA_YELLOW, VGA_DGREY);
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* pause_text = "PAUSED";
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
}

void tetris_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void tetris_draw_game_over(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(15, 5, 50, 15, VGA_RED, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* go_text = "GAME OVER!";
    int len = 0;
    while (go_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 7, go_text[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* labels[] = {
        "Final Score: ",
        "Level Reached: ",
        "Lines Cleared: "
    };
    
    int values[] = {score, level, lines_cleared};
    
    for (int i = 0; i < 3; i++) {
        char stat_text[40];
        int pos = 0;
        
        for (int j = 0; labels[i][j]; j++) {
            stat_text[pos++] = labels[i][j];
        }
        
        int temp = values[i];
        if (temp == 0) {
            stat_text[pos++] = '0';
        } else {
            char digits[10];
            int digit_count = 0;
            while (temp > 0) {
                digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int k = digit_count - 1; k >= 0; k--) {
                stat_text[pos++] = digits[k];
            }
        }
        stat_text[pos] = '\0';
        
        len = 0;
        while (stat_text[len]) len++;
        int x = 40 - len / 2;
        
        for (int j = 0; stat_text[j]; j++) {
            vga_putchr_at(x + j, 10 + i * 2, stat_text[j]);
        }
    }
    
    if (score > high_score) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        const char* new_hs = "*** NEW HIGH SCORE! ***";
        len = 0;
        while (new_hs[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 16, new_hs[i]);
        }
        high_score = score;
        tetris_save_high_score();
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst = "Press any key to return to menu";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 18, inst[i]);
    }
    
    vga_end_batch();
    
    wait_for_char();
}