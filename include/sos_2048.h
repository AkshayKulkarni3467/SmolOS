#ifndef INCLUDE_SMOLOS_2048_H
#define INCLUDE_SMOLOS_2048_H

#include "sos_stdint.h"

#define GRID_SIZE 4
#define TILE_WIDTH 8
#define TILE_HEIGHT 3
#define GRID_OFFSET_X 20
#define GRID_OFFSET_Y 6

typedef enum {
    GAME_2048_MENU,
    GAME_2048_PLAYING,
    GAME_2048_PAUSED,
    GAME_2048_WON,
    GAME_2048_OVER
} Game2048State;

typedef enum {
    DIR_2048_NONE,
    DIR_2048_UP,
    DIR_2048_DOWN,
    DIR_2048_LEFT,
    DIR_2048_RIGHT
} Direction2048;

typedef struct {
    int value;
    int merged;
    int is_new;
    int prev_x;
    int prev_y;
} Tile2048;

typedef struct {
    uint8_t fg;
    uint8_t bg;
} TileColor;

void game_2048_init(void);
void game_2048_reset(void);
void game_2048_spawn_tile(void);
void game_2048_handle_input(void);
int game_2048_move(Direction2048 dir);
int game_2048_can_move(void);
void game_2048_draw_grid(void);
void game_2048_draw_tile(int x, int y, int value, int is_new);
void game_2048_draw_menu(void);
void game_2048_draw_game_over(void);
void game_2048_draw_win_screen(void);
void game_2048_draw_ui(void);
TileColor game_2048_get_tile_color(int value);
void game_2048_save_high_score(void);
void game_2048_load_high_score(void);
void game_2048_game_run(void);
void game_2048_game_cleanup(void);
void game_2048_draw_pause_screen(void);

#endif // INCLUDE_SMOLOS_2048_H