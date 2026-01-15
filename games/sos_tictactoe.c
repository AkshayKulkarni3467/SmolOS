#include "sos_tictactoe.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"

static Board board;
static TicTacToeState game_state;
static TTT_GameMode game_mode;
static int menu_selection;
static int mode_selection;
static int cursor_x, cursor_y;
static CellValue current_player;
static GameResult game_result;
static int exit_requested;
static int needs_redraw;

static PlayerStats player_stats;
static PlayerStats ai_stats;

static int thinking_frame;
static uint32_t last_think_update;

static uint32_t ttt_rand_seed = 54321;

static uint32_t ttt_rand(void) {
    ttt_rand_seed = (ttt_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return ttt_rand_seed;
}


void tictactoe_init(void) {
    game_state = TTT_MENU;
    game_mode = MODE_PVP;
    menu_selection = 0;
    mode_selection = 0;
    exit_requested = 0;
    needs_redraw = 1;
    thinking_frame = 0;
    
    player_stats.wins = 0;
    player_stats.losses = 0;
    player_stats.draws = 0;
    
    ai_stats.wins = 0;
    ai_stats.losses = 0;
    ai_stats.draws = 0;
    
    ttt_load_stats();
}

void ttt_reset_game(void) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            board.cells[y][x] = CELL_EMPTY;
        }
    }
    
    cursor_x = 1;
    cursor_y = 1;
    current_player = CELL_X;
    game_result = RESULT_NONE;
    needs_redraw = 1;
    thinking_frame = 0;
}


void ttt_make_move(int x, int y) {
    if (!ttt_is_valid_move(x, y)) return;
    
    board.cells[y][x] = current_player;
    
    game_result = ttt_check_winner();
    
    if (game_result != RESULT_NONE) {
        game_state = TTT_GAME_OVER;
        
        if (game_result == RESULT_X_WINS) {
            player_stats.wins++;
            if (game_mode != MODE_PVP) ai_stats.losses++;
        } else if (game_result == RESULT_O_WINS) {
            if (game_mode == MODE_PVP) {
                player_stats.wins++;
            } else {
                ai_stats.wins++;
                player_stats.losses++;
            }
        } else {
            player_stats.draws++;
            if (game_mode != MODE_PVP) ai_stats.draws++;
        }
        
        ttt_save_stats();
    } else if (ttt_is_board_full()) {
        game_result = RESULT_DRAW;
        game_state = TTT_GAME_OVER;
        player_stats.draws++;
        if (game_mode != MODE_PVP) ai_stats.draws++;
        ttt_save_stats();
    } else {
        ttt_switch_player();
    }
    
    needs_redraw = 1;
}

int ttt_is_valid_move(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) return 0;
    return board.cells[y][x] == CELL_EMPTY;
}

GameResult ttt_check_winner(void) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        if (board.cells[y][0] != CELL_EMPTY &&
            board.cells[y][0] == board.cells[y][1] &&
            board.cells[y][1] == board.cells[y][2]) {
            return board.cells[y][0] == CELL_X ? RESULT_X_WINS : RESULT_O_WINS;
        }
    }
    
    for (int x = 0; x < BOARD_SIZE; x++) {
        if (board.cells[0][x] != CELL_EMPTY &&
            board.cells[0][x] == board.cells[1][x] &&
            board.cells[1][x] == board.cells[2][x]) {
            return board.cells[0][x] == CELL_X ? RESULT_X_WINS : RESULT_O_WINS;
        }
    }
    
    if (board.cells[0][0] != CELL_EMPTY &&
        board.cells[0][0] == board.cells[1][1] &&
        board.cells[1][1] == board.cells[2][2]) {
        return board.cells[0][0] == CELL_X ? RESULT_X_WINS : RESULT_O_WINS;
    }
    
    if (board.cells[0][2] != CELL_EMPTY &&
        board.cells[0][2] == board.cells[1][1] &&
        board.cells[1][1] == board.cells[2][0]) {
        return board.cells[0][2] == CELL_X ? RESULT_X_WINS : RESULT_O_WINS;
    }
    
    return RESULT_NONE;
}

int ttt_is_board_full(void) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board.cells[y][x] == CELL_EMPTY) return 0;
        }
    }
    return 1;
}

void ttt_switch_player(void) {
    current_player = (current_player == CELL_X) ? CELL_O : CELL_X;
}


Move ttt_ai_get_move(void) {
    switch (game_mode) {
        case MODE_PVE_EASY:
            return ttt_ai_easy();
        case MODE_PVE_MEDIUM:
            return ttt_ai_medium();
        case MODE_PVE_HARD:
            return ttt_ai_hard();
        default:
            return ttt_ai_easy();
    }
}

Move ttt_ai_easy(void) {
    Move move;
    int attempts = 0;
    
    do {
        move.x = ttt_rand() % BOARD_SIZE;
        move.y = ttt_rand() % BOARD_SIZE;
        attempts++;
    } while (!ttt_is_valid_move(move.x, move.y) && attempts < 100);
    
    return move;
}

Move ttt_ai_medium(void) {
    Move move;
    
    if (ttt_rand() % 2 == 0) {
        return ttt_ai_hard();
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (ttt_is_valid_move(x, y)) {
                board.cells[y][x] = CELL_O;
                if (ttt_check_winner() == RESULT_O_WINS) {
                    board.cells[y][x] = CELL_EMPTY;
                    move.x = x;
                    move.y = y;
                    return move;
                }
                board.cells[y][x] = CELL_EMPTY;
            }
        }
    }
    
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (ttt_is_valid_move(x, y)) {
                board.cells[y][x] = CELL_X;
                if (ttt_check_winner() == RESULT_X_WINS) {
                    board.cells[y][x] = CELL_EMPTY;
                    move.x = x;
                    move.y = y;
                    return move;
                }
                board.cells[y][x] = CELL_EMPTY;
            }
        }
    }

    return ttt_ai_easy();
}

Move ttt_ai_hard(void) {
    return ttt_find_best_move();
}

int ttt_evaluate_board(Board* b) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        if (b->cells[y][0] != CELL_EMPTY &&
            b->cells[y][0] == b->cells[y][1] &&
            b->cells[y][1] == b->cells[y][2]) {
            return b->cells[y][0] == CELL_O ? 10 : -10;
        }
    }
    

    for (int x = 0; x < BOARD_SIZE; x++) {
        if (b->cells[0][x] != CELL_EMPTY &&
            b->cells[0][x] == b->cells[1][x] &&
            b->cells[1][x] == b->cells[2][x]) {
            return b->cells[0][x] == CELL_O ? 10 : -10;
        }
    }
    

    if (b->cells[0][0] != CELL_EMPTY &&
        b->cells[0][0] == b->cells[1][1] &&
        b->cells[1][1] == b->cells[2][2]) {
        return b->cells[0][0] == CELL_O ? 10 : -10;
    }
    
    if (b->cells[0][2] != CELL_EMPTY &&
        b->cells[0][2] == b->cells[1][1] &&
        b->cells[1][1] == b->cells[2][0]) {
        return b->cells[0][2] == CELL_O ? 10 : -10;
    }
    
    return 0;
}

int ttt_minimax(Board* b, int depth, int is_maximizing, int alpha, int beta) {
    int score = ttt_evaluate_board(b);
    

    if (score == 10) return score - depth;
    if (score == -10) return score + depth;
    

    int moves_left = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (b->cells[y][x] == CELL_EMPTY) moves_left++;
        }
    }
    if (moves_left == 0) return 0;
    
    if (is_maximizing) {
        int best = -1000;
        
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                if (b->cells[y][x] == CELL_EMPTY) {
                    b->cells[y][x] = CELL_O;
                    int val = ttt_minimax(b, depth + 1, 0, alpha, beta);
                    b->cells[y][x] = CELL_EMPTY;
                    
                    if (val > best) best = val;
                    if (val > alpha) alpha = val;
                    if (beta <= alpha) break;
                }
            }
        }
        return best;
    } else {
        int best = 1000;
        
        for (int y = 0; y < BOARD_SIZE; y++) {
            for (int x = 0; x < BOARD_SIZE; x++) {
                if (b->cells[y][x] == CELL_EMPTY) {
                    b->cells[y][x] = CELL_X;
                    int val = ttt_minimax(b, depth + 1, 1, alpha, beta);
                    b->cells[y][x] = CELL_EMPTY;
                    
                    if (val < best) best = val;
                    if (val < beta) beta = val;
                    if (beta <= alpha) break;
                }
            }
        }
        return best;
    }
}

Move ttt_find_best_move(void) {
    Move best_move;
    best_move.x = -1;
    best_move.y = -1;
    int best_val = -1000;
    
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board.cells[y][x] == CELL_EMPTY) {
                board.cells[y][x] = CELL_O;
                int move_val = ttt_minimax(&board, 0, 0, -1000, 1000);
                board.cells[y][x] = CELL_EMPTY;
                
                if (move_val > best_val) {
                    best_move.x = x;
                    best_move.y = y;
                    best_val = move_val;
                }
            }
        }
    }
    
    return best_move;
}

void ttt_draw_menu(void) {
    vga_begin_batch();
    vga_clear();
    
    const char* title[] = {
        " _____ ___ ____     _____  _    ____     _____ ___  _____ ",
        "|_   _|_ _/ ___|   |_   _|/ \\  / ___|   |_   _/ _ \\| ____|",
        "  | |  | | |   _____ | | / _ \\| |   _____ | || | | |  _|  ",
        "  | |  | | |__|_____|| |/ ___ \\ |__|_____|| || |_| | |___ ",
        "  |_| |___\\____|    |_/_/   \\_\\____|    |_| \\___/|_____|"
    };
    
    uint8_t colors[] = {VGA_LRED, VGA_YELLOW, VGA_LGREEN, VGA_LCYAN, VGA_LBLUE};
    
    for (int i = 0; i < 5; i++) {
        vga_set_color(colors[i], VGA_BLCK);
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 3 + i, title[i][j]);
        }
    }
    
    const char* options[] = {
        "Play Game",
        "View Statistics",
        "Exit"
    };
    
    for (int i = 0; i < 3; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_fill_rect(28, 11 + i * 2, 24, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? " \x10 " : "   ";
        int x = 30;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 11 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 11 + i * 2, options[i][j]);
        }
    }

    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to exit";
    int len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
    
    vga_end_batch();
}

void ttt_draw_mode_select(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* title = "SELECT GAME MODE";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 5, title[i]);
    }
    
    const char* modes[] = {
        "Player vs Player",
        "Player vs AI (Easy)",
        "Player vs AI (Medium)",
        "Player vs AI (Hard)",
        "Back to Menu"
    };
    
    const char* descriptions[] = {
        "Classic two-player mode",
        "AI makes random moves",
        "AI plays defensively",
        "Perfect AI using Minimax",
        ""
    };
    
    for (int i = 0; i < 5; i++) {
        int selected = (i == mode_selection);
        
        if (selected) {
            vga_fill_rect(20, 9 + i * 3, 40, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? " \x10 " : "   ";
        int x = 25;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 9 + i * 3, prefix[j]);
        }
        
        for (int j = 0; modes[i][j]; j++) {
            vga_putchr_at(x++, 9 + i * 3, modes[i][j]);
        }
        
        if (i < 4) {
            vga_set_color(VGA_DGREY, VGA_BLCK);
            len = 0;
            while (descriptions[i][len]) len++;
            for (int j = 0; j < len; j++) {
                vga_putchr_at((80 - len) / 2 + j, 10 + i * 3, descriptions[i][j]);
            }
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to go back";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
    
    vga_end_batch();
}

void ttt_draw_x(int screen_x, int screen_y) {
    vga_set_color(VGA_LRED, VGA_BLCK);
    
    vga_putchr_at(screen_x + 1, screen_y + 1, '\\');
    vga_putchr_at(screen_x + 2, screen_y + 1, ' ');
    vga_putchr_at(screen_x + 3, screen_y + 1, '/');
    
    vga_putchr_at(screen_x + 2, screen_y + 2, 'X');
    
    vga_putchr_at(screen_x + 1, screen_y + 3, '/');
    vga_putchr_at(screen_x + 2, screen_y + 3, ' ');
    vga_putchr_at(screen_x + 3, screen_y + 3, '\\');
}

void ttt_draw_o(int screen_x, int screen_y) {
    vga_set_color(VGA_LBLUE, VGA_BLCK);

    vga_putchr_at(screen_x + 1, screen_y + 1, '_');
    vga_putchr_at(screen_x + 2, screen_y + 1, '_');
    vga_putchr_at(screen_x + 3, screen_y + 1, '_');
    
    vga_putchr_at(screen_x + 1, screen_y + 2, '(');
    vga_putchr_at(screen_x + 2, screen_y + 2, 'O');
    vga_putchr_at(screen_x + 3, screen_y + 2, ')');
    
    vga_putchr_at(screen_x + 1, screen_y + 3, '\\');
    vga_putchr_at(screen_x + 2, screen_y + 3, '_');
    vga_putchr_at(screen_x + 3, screen_y + 3, '/');
}

void ttt_draw_cell(int x, int y) {
    int screen_x = BOARD_OFFSET_X + x * (CELL_WIDTH + 1);
    int screen_y = BOARD_OFFSET_Y + y * (CELL_HEIGHT + 1);

    int is_cursor = (x == cursor_x && y == cursor_y);
    uint8_t bg_color = is_cursor ? VGA_DGREY : VGA_BLCK;

    vga_fill_rect(screen_x, screen_y, CELL_WIDTH, CELL_HEIGHT, ' ', VGA_WHITE, bg_color);

    vga_set_color(VGA_CYAN, bg_color);
    for (int i = 0; i < CELL_WIDTH; i++) {
        vga_putchr_at(screen_x + i, screen_y, 0xC4);
        vga_putchr_at(screen_x + i, screen_y + CELL_HEIGHT - 1, 0xC4);
    }
    for (int i = 0; i < CELL_HEIGHT; i++) {
        vga_putchr_at(screen_x, screen_y + i, 0xB3);
        vga_putchr_at(screen_x + CELL_WIDTH - 1, screen_y + i, 0xB3);
    }
    vga_putchr_at(screen_x, screen_y, 0xDA);
    vga_putchr_at(screen_x + CELL_WIDTH - 1, screen_y, 0xBF);
    vga_putchr_at(screen_x, screen_y + CELL_HEIGHT - 1, 0xC0);
    vga_putchr_at(screen_x + CELL_WIDTH - 1, screen_y + CELL_HEIGHT - 1, 0xD9);

    if (board.cells[y][x] == CELL_X) {
        ttt_draw_x(screen_x + 1, screen_y);
    } else if (board.cells[y][x] == CELL_O) {
        ttt_draw_o(screen_x + 1, screen_y);
    }
}

void ttt_draw_board(void) {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            ttt_draw_cell(x, y);
        }
    }
}

void ttt_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== TIC-TAC-TOE ===", 0);

    const char* mode_names[] = {
        "Player vs Player",
        "vs AI (Easy)",
        "vs AI (Medium)",
        "vs AI (Hard)"
    };
    vga_set_color(VGA_LCYAN, VGA_BLUE);
    vga_print_centered(mode_names[game_mode], 1);

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* turn_text = "Current Turn:";
    int len = 0;
    while (turn_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(10 + i, 5, turn_text[i]);
    }
    
    if (current_player == CELL_X) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_putchr_at(10, 6, 'X');
        vga_putchr_at(11, 6, ' ');
        vga_putchr_at(12, 6, '(');
        vga_putchr_at(13, 6, 'Y');
        vga_putchr_at(14, 6, 'o');
        vga_putchr_at(15, 6, 'u');
        vga_putchr_at(16, 6, ')');
    } else {
        vga_set_color(VGA_LBLUE, VGA_BLCK);
        vga_putchr_at(10, 6, 'O');
        if (game_mode == MODE_PVP) {
            vga_putchr_at(11, 6, ' ');
            vga_putchr_at(12, 6, '(');
            vga_putchr_at(13, 6, 'P');
            vga_putchr_at(14, 6, '2');
            vga_putchr_at(15, 6, ')');
        } else {
            vga_putchr_at(11, 6, ' ');
            vga_putchr_at(12, 6, '(');
            vga_putchr_at(13, 6, 'A');
            vga_putchr_at(14, 6, 'I');
            vga_putchr_at(15, 6, ')');
        }
    }

    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* controls = "Arrows:Move | Enter:Place | ESC:Menu";
    len = 0;
    while (controls[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, controls[i]);
    }
}

void ttt_draw_stats(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(15, 5, 50, 15, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* title = "STATISTICS";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 7, title[i]);
    }

    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* p_title = "YOUR STATS (X)";
    len = 0;
    while (p_title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 9, p_title[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    char stat_text[40];

    int pos = 0;
    const char* wins_label = "Wins: ";
    for (int i = 0; wins_label[i]; i++) stat_text[pos++] = wins_label[i];
    int temp = player_stats.wins;
    if (temp == 0) {
        stat_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stat_text[pos++] = digits[i];
        }
    }
    stat_text[pos] = '\0';
    
    for (int i = 0; stat_text[i]; i++) {
        vga_putchr_at(25 + i, 11, stat_text[i]);
    }

    pos = 0;
    const char* losses_label = "Losses: ";
    for (int i = 0; losses_label[i]; i++) stat_text[pos++] = losses_label[i];
    temp = player_stats.losses;
    if (temp == 0) {
        stat_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stat_text[pos++] = digits[i];
        }
    }
    stat_text[pos] = '\0';
    
    for (int i = 0; stat_text[i]; i++) {
        vga_putchr_at(25 + i, 12, stat_text[i]);
    }

    pos = 0;
    const char* draws_label = "Draws: ";
    for (int i = 0; draws_label[i]; i++) stat_text[pos++] = draws_label[i];
    temp = player_stats.draws;
    if (temp == 0) {
        stat_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stat_text[pos++] = digits[i];
        }
    }
    stat_text[pos] = '\0';
    
    for (int i = 0; stat_text[i]; i++) {
        vga_putchr_at(25 + i, 13, stat_text[i]);
    }

    vga_set_color(VGA_LBLUE, VGA_BLCK);
    const char* ai_title = "AI STATS (O)";
    len = 0;
    while (ai_title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, ai_title[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);

    pos = 0;
    for (int i = 0; wins_label[i]; i++) stat_text[pos++] = wins_label[i];
    temp = ai_stats.wins;
    if (temp == 0) {
        stat_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stat_text[pos++] = digits[i];
        }
    }
    stat_text[pos] = '\0';
    
    for (int i = 0; stat_text[i]; i++) {
        vga_putchr_at(25 + i, 17, stat_text[i]);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Press any key to return";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 19, inst[i]);
    }
    
    vga_end_batch();
    
    wait_for_char();
    needs_redraw = 1;
}

void ttt_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    ttt_draw_ui();
    ttt_draw_board();
    
    vga_end_batch();
}

void ttt_draw_thinking_animation(void) {
    const char* frames[] = {
        "Thinking.  ",
        "Thinking.. ",
        "Thinking..."
    };
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* text = frames[thinking_frame % 3];
    for (int i = 0; text[i]; i++) {
        vga_putchr_at(55 + i, 10, text[i]);
    }
}

void ttt_draw_game_over(void) {
    vga_begin_batch();
    
    int box_w = 40;
    int box_h = 8;
    int box_x = (80 - box_w) / 2;
    int box_y = (25 - box_h) / 2;
    
    uint8_t border_color = VGA_YELLOW;
    uint8_t bg_color = VGA_BLCK;
    
    if (game_result == RESULT_X_WINS) {
        border_color = VGA_LGREEN;
        bg_color = VGA_GREEN;
    } else if (game_result == RESULT_O_WINS) {
        if (game_mode == MODE_PVP) {
            border_color = VGA_LGREEN;
            bg_color = VGA_GREEN;
        } else {
            border_color = VGA_LRED;
            bg_color = VGA_RED;
        }
    } else {
        border_color = VGA_YELLOW;
        bg_color = VGA_BRWN;
    }
    
    vga_draw_box_double(box_x, box_y, box_w, box_h, border_color, VGA_BLCK);
    vga_fill_rect(box_x + 1, box_y + 1, box_w - 2, box_h - 2, ' ', VGA_YELLOW, bg_color);

    vga_set_color(VGA_YELLOW, bg_color);
    const char* title = "";
    
    if (game_result == RESULT_X_WINS) {
        title = " \x01 YOU WIN! \x01 ";
    } else if (game_result == RESULT_O_WINS) {
        if (game_mode == MODE_PVP) {
            title = " \x01 PLAYER 2 WINS! \x01 ";
        } else {
            title = " \x0F AI WINS! \x0F ";
        }
    } else {
        title = "IT'S A DRAW!";
    }
    
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, box_y + 2, title[i]);
    }

    vga_set_color(VGA_WHITE, bg_color);
    char record[40];
    int pos = 0;
    const char* rec_label = "Your Record: ";
    for (int i = 0; rec_label[i]; i++) record[pos++] = rec_label[i];

    int temp = player_stats.wins;
    if (temp == 0) {
        record[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            record[pos++] = digits[i];
        }
    }
    record[pos++] = '-';
    
    temp = player_stats.losses;
    if (temp == 0) {
        record[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            record[pos++] = digits[i];
        }
    }
    record[pos++] = '-';
    
    temp = player_stats.draws;
    if (temp == 0) {
        record[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            record[pos++] = digits[i];
        }
    }
    record[pos] = '\0';
    
    len = 0;
    while (record[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, box_y + 4, record[i]);
    }

    vga_set_color(VGA_LCYAN, bg_color);
    const char* options = "ENTER: Play Again | ESC: Menu";
    len = 0;
    while (options[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, box_y + 6, options[i]);
    }
    
    vga_end_batch();
}


void ttt_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == TTT_MENU) {
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 2;
            needs_redraw = 1;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 2) menu_selection = 0;
            needs_redraw = 1;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                game_state = TTT_MODE_SELECT;
                needs_redraw = 1;
            } else if (menu_selection == 1) {
                ttt_draw_stats();
                game_state = TTT_MENU;
            } else if (menu_selection == 2) {
                exit_requested = 1;
            }
        } else if (c == 27) { 
            exit_requested = 1;
        }
    } else if (game_state == TTT_MODE_SELECT) {
        if (c == 0x11) {  
            mode_selection--;
            if (mode_selection < 0) mode_selection = 4;
            needs_redraw = 1;
        } else if (c == 0x12) {  
            mode_selection++;
            if (mode_selection > 4) mode_selection = 0;
            needs_redraw = 1;
        } else if (c == '\n') {  
            if (mode_selection < 4) {
                game_mode = (TTT_GameMode)mode_selection;
                ttt_reset_game();
                game_state = TTT_PLAYING;
            } else {
                game_state = TTT_MENU;
            }
            needs_redraw = 1;
        } else if (c == 27) {  
            game_state = TTT_MENU;
            needs_redraw = 1;
        }
    } else if (game_state == TTT_PLAYING) {
        if (current_player == CELL_O && game_mode != MODE_PVP) {
            return;  
        }
        
        if (c == 0x11 || c == 'w' || c == 'W') {  
            cursor_y--;
            if (cursor_y < 0) cursor_y = BOARD_SIZE - 1;
            needs_redraw = 1;
        } else if (c == 0x12 || c == 's' || c == 'S') {  
            cursor_y++;
            if (cursor_y >= BOARD_SIZE) cursor_y = 0;
            needs_redraw = 1;
        } else if (c == 0x13 || c == 'a' || c == 'A') {  
            cursor_x--;
            if (cursor_x < 0) cursor_x = BOARD_SIZE - 1;
            needs_redraw = 1;
        } else if (c == 0x14 || c == 'd' || c == 'D') {  
            cursor_x++;
            if (cursor_x >= BOARD_SIZE) cursor_x = 0;
            needs_redraw = 1;
        } else if (c == '\n' || c == ' ') {  
            ttt_make_move(cursor_x, cursor_y);
        } else if (c == 27) {  
            game_state = TTT_MENU;
            needs_redraw = 1;
        }
    } else if (game_state == TTT_GAME_OVER) {
        if (c == '\n' || c == ' ') {  
            ttt_reset_game();
            game_state = TTT_PLAYING;
        } else if (c == 27) {  
            game_state = TTT_MENU;
            needs_redraw = 1;
        }
    }
}


void tictactoe_run(void) {
    tictactoe_init();
    
    vga_set_auto_swap(1);
    
    while (!exit_requested) {
        ttt_handle_input();
        
        if (game_state == TTT_MENU) {
            if (needs_redraw) {
                ttt_draw_menu();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == TTT_MODE_SELECT) {
            if (needs_redraw) {
                ttt_draw_mode_select();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == TTT_PLAYING) {
            if (current_player == CELL_O && game_mode != MODE_PVP && game_result == RESULT_NONE) {
                uint32_t current_time = pit_get_total_milliseconds();
                
                if (current_time - last_think_update >= 300) {
                    thinking_frame++;
                    last_think_update = current_time;
                    needs_redraw = 1;
                }
                
                ttt_draw_game();
                ttt_draw_thinking_animation();
                vga_swap_buffers();

                static uint32_t ai_think_start = 0;
                if (ai_think_start == 0) {
                    ai_think_start = current_time;
                }
                
                if (current_time - ai_think_start >= 800) {  
                    Move ai_move = ttt_ai_get_move();
                    ttt_make_move(ai_move.x, ai_move.y);
                    ai_think_start = 0;
                    thinking_frame = 0;
                }
                
                pit_delay_ms(16);
            } else {
                if (needs_redraw) {
                    ttt_draw_game();
                    needs_redraw = 0;
                }
                pit_delay_ms(50);
            }
        } else if (game_state == TTT_GAME_OVER) {
            if (needs_redraw) {
                ttt_draw_game();
                ttt_draw_game_over();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        }
    }
}

void tictactoe_cleanup(void) {
    vga_set_auto_swap(1);
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


void ttt_save_stats(void) {
    // TODO: Save to FAT16
}

void ttt_load_stats(void) {
    // TODO: Load from FAT16
}