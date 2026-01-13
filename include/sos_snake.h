#ifndef INCLUDE_SMOLOS_SNAKE_H
#define INCLUDE_SMOLOS_SNAKE_H

#include "sos_stdint.h"


#define SNAKE_MAX_LENGTH 500
#define GAME_WIDTH 70
#define GAME_HEIGHT 20
#define GAME_OFFSET_X 5
#define GAME_OFFSET_Y 2


typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;


typedef struct {
    int x;
    int y;
} SnakeSegment;


typedef struct {
    int x;
    int y;
    int points;
    char symbol;
    uint8_t color;
} Food;


typedef enum {
    GAME_MENU,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameState;


typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
    DIFFICULTY_EXPERT
} Difficulty;


void snake_game_init(void);
void snake_game_run(void);
void snake_game_cleanup(void);


void snake_update(void);
void snake_handle_input(void);
int snake_check_collision(void);
void snake_spawn_food(void);
void snake_grow(void);


void snake_draw_game(void);
void snake_draw_border(void);
void snake_draw_snake(void);
void snake_draw_food(void);
void snake_draw_ui(void);
void snake_draw_menu(void);
void snake_draw_pause_screen(void);
void snake_draw_game_over(void);


int snake_get_speed(Difficulty diff);
void snake_save_high_score(void);
void snake_load_high_score(void);

#endif // INCLUDE_SMOLOS_SNAKE_H