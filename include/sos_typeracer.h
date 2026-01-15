#ifndef INCLUDE_SMOLOS_TYPERACER_H
#define INCLUDE_SMOLOS_TYPERACER_H

#include "sos_stdint.h"

#define GAME_WIDTH 76
#define GAME_HEIGHT 20
#define GAME_OFFSET_X 2
#define GAME_OFFSET_Y 3

#define MAX_WORDS 30
#define MAX_PARTICLES 100
#define MAX_EXPLOSIONS 20
#define MAX_WORD_LENGTH 20
#define MAX_INPUT_LENGTH 30

#define WORD_BANK_SIZE 200


#define INPUT_Y (GAME_OFFSET_Y + GAME_HEIGHT + 2)

typedef enum {
    MODE_CLASSIC = 0,
    MODE_SPEED_DEMON,
    MODE_SURVIVAL,
    MODE_ZEN
} TR_GameMode;


typedef enum {
    DIFF_EASY = 0,
    DIFF_MEDIUM,
    DIFF_HARD,
    DIFF_INSANE
} TR_Difficulty;

typedef struct {
    char text[MAX_WORD_LENGTH];
    float x, y;
    float vx, vy;
    int active;
    int length;
    uint8_t color;
    int points;
    int typed_chars;  
    int is_targeted;  
} FallingWord;

typedef struct {
    float x, y;
    float vx, vy;
    int active;
    int lifetime;
    uint8_t color;
    char symbol;
} TypeParticle;

typedef struct {
    float x, y;
    int active;
    int frame;
    int max_frames;
    uint8_t color;
} TypeExplosion;

typedef struct {
    int score;
    int combo;
    int max_combo;
    int words_destroyed;
    int total_chars_typed;
    int correct_chars;
    int incorrect_chars;
    int lives;
    float accuracy;
    int level;
    int wpm;  
} TR_PlayerStats;

typedef enum {
    TYPE_MENU = 0,
    TYPE_PLAYING,
    TYPE_PAUSED,
    TYPE_GAME_OVER,
    TYPE_STATS_SCREEN,
    TYPE_EXIT
} TypeRacerState;

typedef struct {
    int combo_count;
    float multiplier;
    uint32_t last_destroy_time;
    int combo_active;
} ComboSystem;

typedef struct {
    TR_PlayerStats stats;
    ComboSystem combo;
    TR_GameMode mode;
    TR_Difficulty difficulty;
    uint32_t start_time;
    uint32_t game_duration;
    int spawn_rate;
    int word_speed_multiplier;
} GameSession;

void typeracer_game_init(void);
void typeracer_game_run(void);
void typeracer_game_cleanup(void);

void typeracer_reset_game(void);
void typeracer_update_game(void);
void typeracer_update_words(void);
void typeracer_update_particles(void);
void typeracer_update_explosions(void);
void typeracer_update_combo(void);
void typeracer_update_wpm(void);

void typeracer_spawn_word(void);
void typeracer_destroy_word(int index);
void typeracer_process_input(char c);
void typeracer_clear_input(void);
int typeracer_find_matching_word(void);
void typeracer_lose_life(void);

void typeracer_init_word_bank(void);
const char* typeracer_get_random_word(void);
int typeracer_get_word_difficulty(const char* word);

void typeracer_spawn_particles(float x, float y, int count, uint8_t color);
void typeracer_spawn_explosion(float x, float y, uint8_t color);
void typeracer_screen_shake(void);

void typeracer_handle_input(void);

void typeracer_draw_menu(void);
void typeracer_draw_game(void);
void typeracer_draw_field(void);
void typeracer_draw_words(void);
void typeracer_draw_input_area(void);
void typeracer_draw_particles(void);
void typeracer_draw_explosions(void);
void typeracer_draw_ui(void);
void typeracer_draw_combo_meter(void);
void typeracer_draw_stats(void);
void typeracer_draw_pause_screen(void);
void typeracer_draw_game_over(void);
void typeracer_draw_accuracy_bar(int x, int y, float accuracy, int width);

uint32_t typeracer_rand(void);
float typeracer_randf(void);
void typeracer_save_high_score(void);
void typeracer_load_high_score(void);
uint8_t typeracer_get_word_color(int difficulty);
int typeracer_string_length(const char* str);
int typeracer_string_compare(const char* s1, const char* s2, int n);
void typeracer_string_copy(char* dest, const char* src);

#endif // INCLUDE_SMOLOS_TYPERACER_H