#ifndef INCLUDE_SMOLOS_MEMORY_GAME_H
#define INCLUDE_SMOLOS_MEMORY_GAME_H

#include "sos_stdint.h"

#define MAX_CARDS 36
#define CARD_WIDTH 9
#define CARD_HEIGHT 5

typedef enum {
    MEMORY_MENU,
    MEMORY_PLAYING,
    MEMORY_CARD_FLIP,
    MEMORY_MATCH_CHECK,
    MEMORY_PAUSED,
    MEMORY_WON,
    MEMORY_QUIT
} MemoryGameState;

typedef enum {
    DIFFICULTY_MEM_EASY,      
    DIFFICULTY_MEM_MEDIUM,    
    DIFFICULTY_MEM_HARD      
} MemoryDifficulty;

typedef enum {
    CARD_HIDDEN,
    CARD_FLIPPING,
    CARD_REVEALED,
    CARD_MATCHED
} CardState;

typedef struct {
    int symbol;           
    CardState state;
    int matched_with;     
    int flip_timer;       
} MemoryCard;

typedef struct {
    uint32_t moves;
    uint32_t matches;
    uint32_t time_elapsed;
    uint32_t best_time[3];    
    uint32_t best_moves[3];   
} MemoryStats;

void memory_game_init(void);
void memory_game_reset(void);
void memory_game_run(void);
void memory_game_cleanup(void);

void memory_shuffle_cards(void);
void memory_select_card(int index);
void memory_check_match(void);
int memory_all_matched(void);

void memory_handle_input(void);
void memory_handle_mouse_input(void);

void memory_draw_menu(void);
void memory_draw_game(void);
void memory_draw_card(int x, int y, MemoryCard* card, int is_selected);
void memory_draw_ui(void);
void memory_draw_pause_screen(void);
void memory_draw_win_screen(void);
void memory_draw_card_back(int x, int y, int is_selected);
void memory_draw_card_front(int x, int y, int symbol, int is_selected);

uint8_t memory_get_symbol_color(int symbol);
char memory_get_symbol_char(int symbol);
void memory_save_stats(void);
void memory_load_stats(void);

#endif // INCLUDE_SMOLOS_MEMORY_GAME_H