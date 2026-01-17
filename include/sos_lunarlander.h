#ifndef INCLUDE_SMOLOS_LUNAR_LANDER_H
#define INCLUDE_SMOLOS_LUNAR_LANDER_H

#include "sos_stdint.h"

#define TERRAIN_WIDTH 160
#define TERRAIN_HEIGHT 20
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 22
#define VIEWPORT_OFFSET_Y 2

#define GRAVITY 0.05f
#define THRUST_POWER 0.15f
#define MAX_SAFE_VELOCITY 1.0f
#define MAX_SAFE_ANGLE 15.0f

#define SHIP_WIDTH 5
#define SHIP_HEIGHT 3
#define MAX_FUEL 100.0f
#define FUEL_CONSUMPTION 0.5f

#define LANDING_PAD_WIDTH 10
#define LANDING_PAD_MIN_HEIGHT 3

typedef enum {
    LANDER_DIFFICULTY_EASY,
    LANDER_DIFFICULTY_MEDIUM,
    LANDER_DIFFICULTY_HARD,
    LANDER_DIFFICULTY_EXTREME
} LanderDifficulty;

typedef enum {
    LANDER_MENU,
    LANDER_DIFFICULTY_SELECT,
    LANDER_STATISTICS,
    LANDER_PLAYING,
    LANDER_PAUSED,
    LANDER_LANDED,
    LANDER_CRASHED,
    LANDER_EXIT
} LanderState;

typedef enum {
    LANDING_NONE,
    LANDING_PERFECT,    
    LANDING_GOOD,
    LANDING_ROUGH,
    LANDING_CRASHED
} LandingResult;

typedef struct {
    float x, y;          
    float vx, vy;         
    float angle;          
    float fuel;
    int alive;
} Ship;

typedef struct {
    int height[TERRAIN_WIDTH];
    int landing_pad_x;
    int landing_pad_width;
} Terrain;

typedef struct {
    int x, y;
    float vx, vy;
    int lifetime;
    uint8_t color;
} LunarParticle;

typedef struct {
    int landings;
    int crashes;
    int perfect_landings;
    int total_fuel_saved;
    int high_score;
} LanderStats;

void lunar_lander_init(void);
void lunar_lander_run(void);
void lunar_lander_cleanup(void);

void lander_reset_game(void);
void lander_generate_terrain(void);
int lander_generate_terrain_recursive(int x1, int y1, int x2, int y2, int roughness);
void lander_place_landing_pad(void);
void lander_update_game(void);
void lander_update_ship(void);
void lander_apply_thrust(int direction);
void lander_check_collision(void);
LandingResult lander_check_landing(void);
int lander_calculate_score(LandingResult result);

void lander_init_particles(void);
void lander_spawn_particle(float x, float y, float vx, float vy, uint8_t color);
void lander_update_particles(void);
void lander_spawn_exhaust(void);
void lander_spawn_explosion(void);

void lander_draw_menu(void);
void lander_draw_difficulty_select(void);
void lander_draw_game(void);
void lander_draw_terrain(void);
void lander_draw_ship(void);
void lander_draw_particles(void);
void lander_draw_ui(void);
void lander_draw_minimap(void);
void lander_draw_landing_result(void);
void lander_draw_paused(void);

void lander_handle_input(void);

void lander_save_stats(void);
void lander_load_stats(void);
float lander_abs(float x);

#endif // INCLUDE_SMOLOS_LUNAR_LANDER_H