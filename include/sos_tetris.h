#ifndef INCLUDE_SMOLOS_TETRIS_H
#define INCLUDE_SMOLOS_TETRIS_H

#include "sos_stdint.h"

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BOARD_OFFSET_X 30
#define BOARD_OFFSET_Y 2

typedef enum {
    PIECE_I,  
    PIECE_O,  
    PIECE_T, 
    PIECE_S,  
    PIECE_Z,  
    PIECE_J,  
    PIECE_L,  
    PIECE_NONE
} PieceType;

typedef struct {
    PieceType type;
    int x, y;
    int rotation;  
    uint8_t color;
} Piece;

typedef enum {
    TETRIS_MENU,
    TETRIS_PLAYING,
    TETRIS_PAUSED,
    TETRIS_GAME_OVER,
    TETRIS_EXIT,
} TetrisState;

void tetris_game_init(void);
void tetris_game_run(void);
void tetris_game_cleanup(void);

void tetris_spawn_piece(void);
void tetris_move_piece(int dx, int dy);
void tetris_rotate_piece(void);
void tetris_hard_drop(void);
void tetris_lock_piece(void);
int tetris_check_collision(Piece* piece, int dx, int dy);
void tetris_clear_lines(void);
void tetris_update_game(void);

void tetris_draw_menu(void);
void tetris_draw_game(void);
void tetris_draw_board(void);
void tetris_draw_piece(Piece* piece);
void tetris_draw_ghost_piece(void);
void tetris_draw_next_pieces(void);
void tetris_draw_held_piece(void);
void tetris_draw_ui(void);
void tetris_draw_pause_screen(void);
void tetris_draw_game_over(void);

void tetris_handle_input(void);

void tetris_get_piece_blocks(PieceType type, int rotation, int blocks[4][2]);
uint8_t tetris_get_piece_color(PieceType type);
int tetris_calculate_score(int lines_cleared, int level);
void tetris_save_high_score(void);
void tetris_load_high_score(void);

#endif // INCLUDE_SMOLOS_TETRIS_H