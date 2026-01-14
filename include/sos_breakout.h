#ifndef INCLUDE_SMOLOS_BREAKOUT_H
#define INCLUDE_SMOLOS_BREAKOUT_H

#include "sos_stdint.h"

#define GAME_WIDTH 70
#define GAME_HEIGHT 23
#define GAME_OFFSET_X 5
#define GAME_OFFSET_Y 1

#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 1
#define BALL_SIZE 1

#define BRICK_WIDTH 6
#define BRICK_HEIGHT 1
#define BRICK_ROWS 8
#define BRICK_COLS 10
#define BRICK_OFFSET_X 2
#define BRICK_OFFSET_Y 3

#define MAX_LIVES 5
#define POWERUP_DURATION 10000  

typedef enum {
    BRICK_NONE = 0,
    BRICK_RED = 1,      
    BRICK_ORANGE = 2,   
    BRICK_YELLOW = 3,   
    BRICK_GREEN = 4,    
    BRICK_BLUE = 5,     
    BRICK_SILVER = 6,   
    BRICK_GOLD = 7      
} BrickType;

typedef enum {
    POWERUP_NONE = 0,
    POWERUP_EXPAND,     
    POWERUP_SHRINK,     
    POWERUP_SLOW,       
    POWERUP_FAST,       
    POWERUP_MULTIBALL,  
    POWERUP_LASER,      
    POWERUP_LIFE        
} PowerupType;

typedef enum {
    BREAKOUT_MENU,
    BREAKOUT_LEVEL_SELECT,
    BREAKOUT_PLAYING,
    BREAKOUT_PAUSED,
    BREAKOUT_LEVEL_COMPLETE,
    BREAKOUT_GAME_OVER,
    BREAKOUT_EXIT
} BreakoutState;

typedef struct {
    BrickType type;
    int hits_remaining;
    uint8_t color;
    int visible;
} Brick;

typedef struct {
    float x, y;
    int width;
    uint8_t color;
    int laser_active;
    uint32_t laser_end_time;
} Breakout_Paddle;

typedef struct {
    float x, y;
    float vx, vy;
    uint8_t color;
    int active;
} Breakout_Ball;

typedef struct {
    float x, y;
    PowerupType type;
    float vy;
    int active;
    uint8_t color;
} Powerup;

typedef struct {
    float x, y;
    float vy;
    int active;
} Laser;

void breakout_game_init(void);
void breakout_game_run(void);
void breakout_game_cleanup(void);

void breakout_reset_game(void);
void breakout_start_level(int level);
void breakout_reset_ball(void);
void breakout_update_game(void);
void breakout_update_balls(void);
void breakout_update_paddle(void);
void breakout_update_powerups(void);
void breakout_update_lasers(void);
void breakout_check_brick_collision(Breakout_Ball* ball);
void breakout_check_paddle_collision(Breakout_Ball* ball);
void breakout_check_powerup_collision(void);
void breakout_activate_powerup(PowerupType type);
void breakout_spawn_powerup(int brick_x, int brick_y);
void breakout_lose_life(void);
void breakout_check_level_complete(void);
void breakout_fire_laser(void);

void breakout_draw_menu(void);
void breakout_draw_level_select(void);
void breakout_draw_game(void);
void breakout_draw_field(void);
void breakout_draw_bricks(void);
void breakout_draw_paddle(void);
void breakout_draw_balls(void);
void breakout_draw_powerups(void);
void breakout_draw_lasers(void);
void breakout_draw_ui(void);
void breakout_draw_lives(void);
void breakout_draw_pause_screen(void);
void breakout_draw_level_complete(void);
void breakout_draw_game_over(void);

void breakout_handle_input(void);

uint8_t breakout_get_brick_color(BrickType type);
int breakout_get_brick_points(BrickType type);
void breakout_save_high_score(void);
void breakout_load_high_score(void);

#endif // INCLUDE_SMOLOS_BREAKOUT_H