#ifndef INCLUDE_SMOLOS_TICTACTOE_H
#define INCLUDE_SMOLOS_TICTACTOE_H

#include "sos_stdint.h"

#define BOARD_SIZE 3
#define CELL_WIDTH 8
#define CELL_HEIGHT 4
#define BOARD_OFFSET_X 28
#define BOARD_OFFSET_Y 8

typedef enum {
    CELL_EMPTY = 0,
    CELL_X = 1,
    CELL_O = 2
} CellValue;

typedef enum {
    MODE_PVP,       
    MODE_PVE_EASY,  
    MODE_PVE_MEDIUM,
    MODE_PVE_HARD   
} TTT_GameMode;

typedef enum {
    TTT_MENU,
    TTT_MODE_SELECT,
    TTT_PLAYING,
    TTT_GAME_OVER,
    TTT_EXIT
} TicTacToeState;

typedef enum {
    PLAYER_HUMAN,
    PLAYER_AI
} PlayerType;

typedef enum {
    RESULT_NONE,
    RESULT_X_WINS,
    RESULT_O_WINS,
    RESULT_DRAW
} GameResult;

typedef struct {
    CellValue cells[BOARD_SIZE][BOARD_SIZE];
} Board;

typedef struct {
    int x, y;
    int score;
} Move;

typedef struct {
    int wins;
    int losses;
    int draws;
} PlayerStats;

void tictactoe_init(void);
void tictactoe_run(void);
void tictactoe_cleanup(void);

void ttt_reset_game(void);
void ttt_make_move(int x, int y);
int ttt_is_valid_move(int x, int y);
GameResult ttt_check_winner(void);
int ttt_is_board_full(void);
void ttt_switch_player(void);

Move ttt_ai_get_move(void);
Move ttt_ai_easy(void);
Move ttt_ai_medium(void);
Move ttt_ai_hard(void);
int ttt_minimax(Board* board, int depth, int is_maximizing, int alpha, int beta);
Move ttt_find_best_move(void);
int ttt_evaluate_board(Board* board);

void ttt_draw_menu(void);
void ttt_draw_mode_select(void);
void ttt_draw_game(void);
void ttt_draw_board(void);
void ttt_draw_cell(int x, int y);
void ttt_draw_x(int screen_x, int screen_y);
void ttt_draw_o(int screen_x, int screen_y);
void ttt_draw_ui(void);
void ttt_draw_stats(void);
void ttt_draw_game_over(void);
void ttt_draw_thinking_animation(void);

void ttt_handle_input(void);

void ttt_save_stats(void);
void ttt_load_stats(void);

#endif // INCLUDE_SMOLOS_TICTACTOE_H