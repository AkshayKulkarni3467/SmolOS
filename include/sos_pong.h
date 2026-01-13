#ifndef INCLUDE_SMOLOS_PONG_H
#define INCLUDE_SMOLOS_PONG_H

#include "sos_stdint.h"

#define GAME_WIDTH 70
#define GAME_HEIGHT 20
#define GAME_OFFSET_X 5
#define GAME_OFFSET_Y 2

#define PADDLE_HEIGHT 4
#define PADDLE_WIDTH 1
#define BALL_SIZE 1

#define MAX_SCORE 3
#define BALL_SPEED_INCREMENT 10
#define AI_DIFFICULTY_EASY 70
#define AI_DIFFICULTY_MEDIUM 85
#define AI_DIFFICULTY_HARD 95


typedef enum {
    MODE_SINGLE_PLAYER,
    MODE_TWO_PLAYER
} GameMode;


typedef enum {
    DIFFICULTY_PONG_EASY,
    DIFFICULTY_PONG_MEDIUM,
    DIFFICULTY_PONG_HARD,
} Difficulty_Pong;


typedef enum {
    PONG_MENU,
    PONG_DIFFICULTY_SELECT,
    PONG_PLAYING,
    PONG_PAUSED,
    PONG_GAME_OVER,
    PONG_EXIT
} PongState;


typedef struct {
    int x, y;
    int height;
    uint8_t color;
    int score;
} Paddle;


typedef struct {
    float x, y;
    float vx, vy;
    uint8_t color;
    int speed;
} Ball;


void pong_game_init(void);
void pong_game_run(void);
void pong_game_cleanup(void);


void pong_reset_game(void);
void pong_reset_ball(int direction);
void pong_update_game(void);
void pong_update_ball(void);
void pong_update_paddles(void);
void pong_ai_move(void);
void pong_check_collisions(void);
void pong_check_scoring(void);


void pong_draw_menu(void);
void pong_draw_difficulty_menu(void);
void pong_draw_game(void);
void pong_draw_field(void);
void pong_draw_paddles(void);
void pong_draw_ball(void);
void pong_draw_ui(void);
void pong_draw_pause_screen(void);
void pong_draw_game_over(void);
void pong_draw_controls(void);


void pong_handle_input(void);


void pong_save_high_score(void);
void pong_load_high_score(void);
int pong_abs(int x);

#endif // INCLUDE_SMOLOS_PONG_H