#ifndef INCLUDE_SMOLOS_MINESWEEPER_H
#define INCLUDE_SMOLOS_MINESWEEPER_H

#include "sos_stdint.h"

#define BOARD_WIDTH 30
#define BOARD_HEIGHT 16
#define MAX_MINES 99
#define BOARD_OFFSET_X 2
#define BOARD_OFFSET_Y 4

typedef enum {
    CELL_HIDDEN = 0,
    CELL_REVEALED,
    CELL_FLAGGED,
    CELL_QUESTION
} CellState;

typedef struct {
    int has_mine;
    int adjacent_mines;
    CellState state;
} Cell;

typedef enum {
    DIFF_BEGINNER = 0,
    DIFF_INTERMEDIATE,
    DIFF_EXPERT,
    DIFF_CUSTOM
} GameDifficulty;

typedef enum {
    GAME_MINESWEEP_MENU = 0,
    GAME_MINESWEEP_PLAYING,
    GAME_MINESWEEP_WON,
    GAME_MINESWEEP_LOST,
    GAME_MINESWEEP_PAUSED,
    GAME_MINESWEEP_QUIT
} MinesweeperGameState;

typedef struct {
    int width;
    int height;
    int mine_count;
} BoardConfig;

typedef struct {
    int x;
    int y;
} CursorPos;

void minesweeper_game_init(void);
void minesweeper_game_run(void);
void minesweeper_game_cleanup(void);

void minesweeper_reset_game(void);
void minesweeper_generate_board(int first_x, int first_y);
void minesweeper_reveal_cell(int x, int y);
void minesweeper_toggle_flag(int x, int y);
void minesweeper_toggle_question(int x, int y);
void minesweeper_reveal_all_mines(void);
int minesweeper_check_win(void);
int minesweeper_count_adjacent_mines(int x, int y);
void minesweeper_reveal_adjacent(int x, int y);

void minesweeper_handle_input(void);

void minesweeper_draw_menu(void);
void minesweeper_draw_game(void);
void minesweeper_draw_board(void);
void minesweeper_draw_cell(int x, int y);
void minesweeper_draw_ui(void);
void minesweeper_draw_win_screen(void);
void minesweeper_draw_lose_screen(void);
void minesweeper_draw_pause_screen(void);

void minesweeper_set_difficulty(GameDifficulty diff);
BoardConfig minesweeper_get_config(GameDifficulty diff);
uint32_t minesweeper_rand(void);
void minesweeper_format_time(uint32_t seconds, char* buffer);

void minesweeper_save_stats(void);
void minesweeper_load_stats(void);

#endif // INCLUDE_SMOLOS_MINESWEEPER_H