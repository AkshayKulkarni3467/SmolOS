#ifndef INCLUDE_SMOLOS_SPACESHOOTER_H
#define INCLUDE_SMOLOS_SPACESHOOTER_H

#include "sos_stdint.h"

#define SS_GAME_WIDTH 60
#define SS_GAME_HEIGHT 22
#define SS_GAME_OFFSET_X 10
#define SS_GAME_OFFSET_Y 2

#define SS_PI        3.14159265358979323846f
#define SS_TWO_PI    6.28318530717958647692f
#define SS_HALF_PI   1.57079632679489661923f

#define MAX_BULLETS 50
#define MAX_ENEMIES 30
#define MAX_POWERUPS 10
#define MAX_EXPLOSIONS 20
#define MAX_PARTICLES 100
#define MAX_STARS 50

#define PLAYER_START_X (SS_GAME_WIDTH / 2)
#define PLAYER_START_Y (SS_GAME_HEIGHT - 4)
#define PLAYER_MAX_HEALTH 100
#define PLAYER_SPEED 3

#define BULLET_SPEED 1.0f
#define ENEMY_SPAWN_RATE 80
#define POWERUP_DURATION 10000  

typedef struct {
    float x, y;
    float vx, vy;
    int active;
    uint8_t color;
    uint8_t damage;
    int is_player_bullet;
} Bullet;

typedef struct {
    float x, y;
    float vx, vy;
    int active;
    uint8_t color;
    char symbol;
    int health;
    int max_health;
    int type;
    int shoot_timer;
    int points;
    float angle;  
} Enemy;

typedef struct {
    float x, y;
    float vy;
    int active;
    uint8_t color;
    int type;
    char symbol;
} SS_Powerup;

typedef struct {
    float x, y;
    int active;
    int frame;
    int max_frames;
    uint8_t color;
} Explosion;

typedef struct {
    float x, y;
    float vx, vy;
    int active;
    int lifetime;
    uint8_t color;
    char symbol;
} Particle;

typedef struct {
    float x, y;
    float vy;
    uint8_t color;
    char symbol;
} Star;

typedef struct {
    float x, y;
    int health;
    int max_health;
    int score;
    int lives;
    uint8_t color;
    int shoot_cooldown;
    int invulnerable_timer;
    int weapon_level;
    int weapon_type;
    int shield_active;
    uint32_t shield_end_time;
    int rapid_fire;
    uint32_t rapid_fire_end_time;
} Player;

typedef struct {
    float x, y;
    int active;
    int health;
    int max_health;
    int phase;
    int attack_timer;
    int move_timer;
    float target_x;
    uint8_t color;
} Boss;

typedef enum {
    SHOOTER_MENU = 0,
    SHOOTER_PLAYING,
    SHOOTER_PAUSED,
    SHOOTER_GAME_OVER,
    SHOOTER_BOSS_FIGHT,
    SHOOTER_LEVEL_COMPLETE,
    SHOOTER_EXIT
} ShooterState;

typedef enum {
    ENEMY_BASIC = 0,
    ENEMY_FAST,
    ENEMY_TANK,
    ENEMY_ZIGZAG,
    ENEMY_SHOOTER,
    ENEMY_KAMIKAZE,
    ENEMY_SPINNER
} EnemyType;

typedef enum {
    SS_POWERUP_HEALTH = 0,
    SS_POWERUP_WEAPON_UP,
    SS_POWERUP_SHIELD,
    SS_POWERUP_RAPID_FIRE,
    SS_POWERUP_SPREAD_SHOT,
    SS_POWERUP_LASER,
    SS_POWERUP_LIFE
} SS_PowerupType;

typedef struct {
    int wave_number;
    int enemies_spawned;
    int enemies_killed;
    int enemies_to_spawn;
    int spawn_timer;
    int boss_wave;
} WaveSystem;

void spaceshooter_game_init(void);
void spaceshooter_game_run(void);
void spaceshooter_game_cleanup(void);

void spaceshooter_reset_game(void);
void spaceshooter_update_game(void);
void spaceshooter_update_player(void);
void spaceshooter_update_bullets(void);
void spaceshooter_update_enemies(void);
void spaceshooter_update_powerups(void);
void spaceshooter_update_explosions(void);
void spaceshooter_update_particles(void);
void spaceshooter_update_stars(void);
void spaceshooter_update_boss(void);
void spaceshooter_update_wave_system(void);

void spaceshooter_move_player(int dx, int dy);
void spaceshooter_player_shoot(void);
void spaceshooter_lose_life(void);

void spaceshooter_spawn_enemy(int type);
void spaceshooter_spawn_powerup(float x, float y, int type);
void spaceshooter_spawn_explosion(float x, float y, uint8_t color);
void spaceshooter_spawn_particles(float x, float y, int count, uint8_t color);
void spaceshooter_spawn_bullet(float x, float y, float vx, float vy, int is_player, uint8_t color, int damage);
void spaceshooter_spawn_boss(void);

void spaceshooter_check_bullet_collisions(void);
void spaceshooter_check_enemy_collisions(void);
void spaceshooter_check_powerup_collisions(void);
void spaceshooter_check_boss_collisions(void);

void spaceshooter_handle_input(void);

void spaceshooter_draw_menu(void);
void spaceshooter_draw_game(void);
void spaceshooter_draw_field(void);
void spaceshooter_draw_player(void);
void spaceshooter_draw_bullets(void);
void spaceshooter_draw_enemies(void);
void spaceshooter_draw_powerups(void);
void spaceshooter_draw_explosions(void);
void spaceshooter_draw_particles(void);
void spaceshooter_draw_stars(void);
void spaceshooter_draw_boss(void);
void spaceshooter_draw_ui(void);
void spaceshooter_draw_health_bar(int x, int y, int health, int max_health, int width);
void spaceshooter_draw_pause_screen(void);
void spaceshooter_draw_game_over(void);
void spaceshooter_draw_level_complete(void);

uint32_t spaceshooter_rand(void);
float spaceshooter_randf(void);
float spaceshooter_abs(float x);
int spaceshooter_get_enemy_health(int type);
int spaceshooter_get_enemy_points(int type);
uint8_t spaceshooter_get_enemy_color(int type);
char spaceshooter_get_enemy_symbol(int type);
void spaceshooter_save_high_score(void);
void spaceshooter_load_high_score(void);

#endif // INCLUDE_SMOLOS_SPACESHOOTER_H