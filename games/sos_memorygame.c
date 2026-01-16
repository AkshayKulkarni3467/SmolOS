#include "sos_memorygame.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_mouse.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_MEMORY_SAVE_FILE "MEMGSAVE.DAT"

static MemoryCard cards[MAX_CARDS];
static MemoryGameState game_state;
static MemoryDifficulty difficulty;
static MemoryStats stats;
static int menu_selection;
static int needs_redraw;

static int grid_cols;
static int grid_rows;
static int total_cards;
static int grid_offset_x;
static int grid_offset_y;

static int selected_card;
static int first_selection;
static int second_selection;
static uint32_t flip_timer;
static uint32_t start_time;

static int cursor_pos;

static uint32_t memory_rand_seed = 12345;

static uint32_t memory_rand(void) {
    memory_rand_seed = (memory_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return memory_rand_seed;
}


void memory_game_init(void) {
    game_state = MEMORY_MENU;
    difficulty = DIFFICULTY_MEM_EASY;
    menu_selection = 0;
    needs_redraw = 1;
    
    selected_card = -1;
    first_selection = -1;
    second_selection = -1;
    cursor_pos = 0;
    
    stats.moves = 0;
    stats.matches = 0;
    stats.time_elapsed = 0;
    
    for (int i = 0; i < 3; i++) {
        stats.best_time[i] = 9999;
        stats.best_moves[i] = 9999;
    }
    
    memory_load_stats();
}

void memory_game_reset(void) {
    switch (difficulty) {
        case DIFFICULTY_MEM_EASY:
            grid_cols = 4;
            grid_rows = 3;
            break;
        case DIFFICULTY_MEM_MEDIUM:
            grid_cols = 4;
            grid_rows = 4;
            break;
        case DIFFICULTY_MEM_HARD:
            grid_cols = 6;
            grid_rows = 4;
            break;
    }
    
    total_cards = grid_cols * grid_rows;

    grid_offset_x = (80 - (grid_cols * (CARD_WIDTH + 1))) / 2;
    grid_offset_y = (25 - (grid_rows * (CARD_HEIGHT + 1))) / 2 + 1;

    int num_pairs = total_cards / 2;
    for (int i = 0; i < total_cards; i++) {
        cards[i].symbol = i / 2;  
        cards[i].state = CARD_HIDDEN;
        cards[i].matched_with = -1;
        cards[i].flip_timer = 0;
    }

    memory_shuffle_cards();

    stats.moves = 0;
    stats.matches = 0;
    stats.time_elapsed = 0;
    selected_card = -1;
    first_selection = -1;
    second_selection = -1;
    cursor_pos = 0;
    needs_redraw = 1;
    start_time = pit_get_seconds();
}

void memory_shuffle_cards(void) {
    for (int i = total_cards - 1; i > 0; i--) {
        int j = memory_rand() % (i + 1);

        MemoryCard temp = cards[i];
        cards[i] = cards[j];
        cards[j] = temp;
    }
}


void memory_select_card(int index) {
    if (index < 0 || index >= total_cards) return;
    if (cards[index].state != CARD_HIDDEN) return;

    cards[index].state = CARD_REVEALED;
    cards[index].flip_timer = 5;
    
    if (first_selection == -1) {
        first_selection = index;
    } else if (second_selection == -1 && index != first_selection) {
        second_selection = index;
        stats.moves++;
        game_state = MEMORY_MATCH_CHECK;
        flip_timer = pit_get_ticks();
        needs_redraw = 1;
    }
}

void memory_check_match(void) {
    if (first_selection == -1 || second_selection == -1) {
        game_state = MEMORY_PLAYING;
        return;
    }

    pit_delay_ms(1000); 
    if (cards[first_selection].symbol == cards[second_selection].symbol) {
        cards[first_selection].state = CARD_MATCHED;
        cards[second_selection].state = CARD_MATCHED;
        cards[first_selection].matched_with = second_selection;
        cards[second_selection].matched_with = first_selection;
        stats.matches++;
        

        if (memory_all_matched()) {
            stats.time_elapsed = pit_get_seconds() - start_time;
            memory_save_stats();
            game_state = MEMORY_WON;
        } else {
            game_state = MEMORY_PLAYING;
        }
    } else {
        cards[first_selection].state = CARD_HIDDEN;
        cards[second_selection].state = CARD_HIDDEN;
        game_state = MEMORY_PLAYING;
    }

    first_selection = -1;
    second_selection = -1;
    needs_redraw = 1;
}

int memory_all_matched(void) {
    for (int i = 0; i < total_cards; i++) {
        if (cards[i].state != CARD_MATCHED) {
            return 0;
        }
    }
    return 1;
}


void memory_handle_mouse_input(void) {
    MouseEvent event;
    while (mouse_get_event(&event)) {
        if (event.type == MOUSE_EVENT_BUTTON_PRESS && (event.buttons & MOUSE_LEFT_BUTTON)) {
            int rel_x = event.x - grid_offset_x;
            int rel_y = event.y - grid_offset_y;
            
            if (rel_x >= 0 && rel_y >= 0) {
                int card_col = rel_x / (CARD_WIDTH + 1);
                int card_row = rel_y / (CARD_HEIGHT + 1);
                
                if (card_col < grid_cols && card_row < grid_rows) {
                    int index = card_row * grid_cols + card_col;

                    int card_x = grid_offset_x + card_col * (CARD_WIDTH + 1);
                    int card_y = grid_offset_y + card_row * (CARD_HEIGHT + 1);
                    
                    if (event.x >= card_x && event.x < card_x + CARD_WIDTH &&
                        event.y >= card_y && event.y < card_y + CARD_HEIGHT) {
                        cursor_pos = index;
                        memory_select_card(index);
                        needs_redraw = 1;
                    }
                }
            }
        }
    }
}

void memory_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == MEMORY_MENU) {
        if (c == 0x11) {  
            menu_selection = (menu_selection - 1 + 4) % 4;
            needs_redraw = 1;
        } else if (c == 0x12) {  
            menu_selection = (menu_selection + 1) % 4;
            needs_redraw = 1;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                memory_game_reset();
                game_state = MEMORY_PLAYING;
            } else if (menu_selection == 1) {
                difficulty = DIFFICULTY_MEM_EASY;
                needs_redraw = 1;
            } else if (menu_selection == 2) {
                difficulty = DIFFICULTY_MEM_MEDIUM;
                needs_redraw = 1;
            } else if (menu_selection == 3) {
                difficulty = DIFFICULTY_MEM_HARD;
                needs_redraw = 1;
            }
        } else if (c == 27) {  
            game_state = MEMORY_QUIT;
        }
    }else if (game_state == MEMORY_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = MEMORY_PLAYING;
            needs_redraw = 1;
        } else if (c == 27) {  
            game_state = MEMORY_MENU;
            needs_redraw = 1;
        }
    }else if (game_state == MEMORY_PLAYING){
        if (c == 'p' || c == 'P') {  
            game_state = MEMORY_PAUSED;
            needs_redraw = 1;
        } else if (c == 27) {  
            game_state = MEMORY_MENU;
            needs_redraw = 1;
        }
    }
     else if (game_state == MEMORY_WON) {
        if (c == '\n' || c == ' ') {
            memory_game_reset();
            game_state = MEMORY_PLAYING;
        } else if (c == 27) {
            game_state = MEMORY_MENU;
            needs_redraw = 1;
        }
    }
}


uint8_t memory_get_symbol_color(int symbol) {
    uint8_t colors[] = {
        VGA_LRED, VGA_LGREEN, VGA_LBLUE, VGA_YELLOW,
        VGA_LMAGENTA, VGA_LCYAN, VGA_RED, VGA_GREEN,
        VGA_BLUE, VGA_MAGENTA, VGA_CYAN, VGA_LRED
    };
    return colors[symbol % 12];
}

char memory_get_symbol_char(int symbol) {
    char symbols[] = {
        0x03, 0x04, 0x05, 0x06,  
        0x0F, 0x10, 0x11, 0x13,  
        0x01, 0x02, 0x14, 0x15   
    };
    return symbols[symbol % 12];
}

void memory_draw_card_back(int x, int y, int is_selected) {
    uint8_t border_color = is_selected ? VGA_YELLOW : VGA_CYAN;
    uint8_t bg_color = VGA_BLUE;

    vga_fill_rect(x, y, CARD_WIDTH, CARD_HEIGHT, ' ', VGA_LCYAN, bg_color);

    vga_draw_box_single(x, y, CARD_WIDTH, CARD_HEIGHT, border_color, VGA_BLCK);

    vga_set_color(VGA_LCYAN, bg_color);
    for (int i = 1; i < CARD_HEIGHT - 1; i++) {
        for (int j = 1; j < CARD_WIDTH - 1; j++) {
            if ((i + j) % 2 == 0) {
                vga_putchr_at(x + j, y + i, 0xB0);  
            } else {
                vga_putchr_at(x + j, y + i, 0xB1);  
            }
        }
    }
}

void memory_draw_card_front(int x, int y, int symbol, int is_selected) {
    uint8_t symbol_color = memory_get_symbol_color(symbol);
    uint8_t border_color = is_selected ? VGA_YELLOW : VGA_WHITE;
    uint8_t bg_color = VGA_LGREY;

    vga_fill_rect(x, y, CARD_WIDTH, CARD_HEIGHT, ' ', symbol_color, bg_color);

    vga_draw_box_single(x, y, CARD_WIDTH, CARD_HEIGHT, border_color, VGA_BLCK);

    char sym = memory_get_symbol_char(symbol);
    int center_x = x + CARD_WIDTH / 2;
    int center_y = y + CARD_HEIGHT / 2;
    
    vga_set_color(symbol_color, bg_color);

    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            vga_putchr_at(center_x + dx, center_y + dy, sym);
        }
    }
}

void memory_draw_card(int x, int y, MemoryCard* card, int is_selected) {
    if (card->state == CARD_HIDDEN) {
        memory_draw_card_back(x, y, is_selected);
    } else if (card->state == CARD_REVEALED || card->state == CARD_MATCHED) {
        memory_draw_card_front(x, y, card->symbol, is_selected);

        if (card->state == CARD_MATCHED) {
            vga_set_color(VGA_YELLOW, VGA_LGREY);
            vga_putchr_at(x + 1, y + 1, 0x0F);
            vga_putchr_at(x + CARD_WIDTH - 2, y + 1, 0x0F);
            vga_putchr_at(x + 1, y + CARD_HEIGHT - 2, 0x0F);
            vga_putchr_at(x + CARD_WIDTH - 2, y + CARD_HEIGHT - 2, 0x0F);
        }
    }
}


void memory_draw_ui(void) {
    char buf[32];
    int pos, temp;

    vga_fill_rect(0, 0, 80, 2, ' ', VGA_YELLOW, VGA_MAGENTA);
    vga_set_color(VGA_YELLOW, VGA_MAGENTA);
    vga_print_centered("*** MEMORY MATCH GAME ***", 0);

    const char* diff_names[] = {"Easy", "Medium", "Hard"};
    vga_set_color(VGA_LCYAN, VGA_MAGENTA);
    vga_print_centered(diff_names[difficulty], 1);

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* moves_label = "Moves:";
    for (int i = 0; moves_label[i]; i++)
        vga_putchr_at(2 + i, 23, moves_label[i]);

    pos = 0;
    temp = stats.moves;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    vga_set_color(VGA_LGREEN, VGA_BLCK);
    for (int i = 0; buf[i]; i++)
        vga_putchr_at(9 + i, 23, buf[i]);

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* pairs_label = "Pairs:";
    for (int i = 0; pairs_label[i]; i++)
        vga_putchr_at(18 + i, 23, pairs_label[i]);

    pos = 0;
    temp = stats.matches;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos++] = '/';

    temp = total_cards / 2;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    for (int i = 0; buf[i]; i++)
        vga_putchr_at(25 + i, 23, buf[i]);

    uint32_t elapsed = pit_get_seconds() - start_time;
    int mins = elapsed / 60;
    int secs = elapsed % 60;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* time_label = "Time:";
    for (int i = 0; time_label[i]; i++)
        vga_putchr_at(40 + i, 23, time_label[i]);

    buf[0] = '0' + (mins / 10);
    buf[1] = '0' + (mins % 10);
    buf[2] = ':';
    buf[3] = '0' + (secs / 10);
    buf[4] = '0' + (secs % 10);
    buf[5] = 0;

    vga_set_color(VGA_LMAGENTA, VGA_BLCK);
    for (int i = 0; buf[i]; i++)
        vga_putchr_at(46 + i, 23, buf[i]);

    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls = "ARROWS:Move  SPACE:Select  P:Pause  ESC:Menu";
    for (int i = 0; controls[i] && i < 80; i++)
        vga_putchr_at(i, 24, controls[i]);
}



void memory_save_stats(void) {
    typedef struct {
        int magic_number;     
        MemoryStats sstats;
        int version;           
        int checksum;
    } GameMemSaveData;
    
    GameMemSaveData save_data;
    save_data.magic_number = 0x91919191;  
    save_data.sstats = stats;
    save_data.version = 1;
    
    save_data.checksum = save_data.magic_number + 
                         save_data.sstats.best_time + 
                         save_data.version;
    
    fat16_write_file(GAME_MEMORY_SAVE_FILE, 
                     (const char*)&save_data, 
                     sizeof(GameMemSaveData));
}

void memory_load_stats(void) {
    if (!fat16_file_exists(GAME_MEMORY_SAVE_FILE)) {
        stats = (MemoryStats){
            .moves = 0,
            .matches = 0,
            .time_elapsed = 0,
            .best_time = {999,999,999},
            .best_moves = {999,999,999},
        };
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_MEMORY_SAVE_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        stats = (MemoryStats){
            .moves = 0,
            .matches = 0,
            .time_elapsed = 0,
            .best_time = {999,999,999},
            .best_moves = {999,999,999},
        };
        return;
    }
    
    typedef struct {
        int magic_number;     
        MemoryStats sstats;
        int version;           
        int checksum;
    } GameMemSaveData;
    
    if (file_size < sizeof(GameMemSaveData)) {
        stats = (MemoryStats){
            .moves = 0,
            .matches = 0,
            .time_elapsed = 0,
            .best_time = {999,999,999},
            .best_moves = {999,999,999},
        };
        return;
    }
    
    GameMemSaveData* save_data = (GameMemSaveData*)file_content;
    
    if (save_data->magic_number != 0x91919191) {
        stats = (MemoryStats){
            .moves = 0,
            .matches = 0,
            .time_elapsed = 0,
            .best_time = {999,999,999},
            .best_moves = {999,999,999},
        };
        return;
    }
    
    int calculated_checksum = save_data->magic_number + 
                             save_data->sstats.best_time + 
                             save_data->version;
    
    if (calculated_checksum != save_data->checksum) {
        stats = (MemoryStats){
            .moves = 0,
            .matches = 0,
            .time_elapsed = 0,
            .best_time = {999,999,999},
            .best_moves = {999,999,999},
        };
        return;
    }
    
    stats = save_data->sstats;
}


void memory_game_run(void) {
    memory_game_init();
    memory_rand_seed = pit_get_ticks();
    
    vga_set_auto_swap(1);
    mouse_show_cursor();
    mouse_set_cursor_char(0x1A);
    
    while (game_state != MEMORY_QUIT) {
        memory_handle_input();
        
        if (game_state == MEMORY_MENU) {
            if (needs_redraw) {
                memory_draw_menu();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == MEMORY_PLAYING) {
            memory_handle_mouse_input();
            
            if (needs_redraw) {
                memory_draw_game();
                needs_redraw = 0;
            }

            static uint32_t last_time_update = 0;
            uint32_t current_time = pit_get_seconds();
            if (current_time != last_time_update) {
                last_time_update = current_time;
                needs_redraw = 1;
            }
            
            pit_delay_ms(50);
        } else if (game_state == MEMORY_MATCH_CHECK) {
            memory_check_match();
            
            if (needs_redraw) {
                memory_draw_game();
                needs_redraw = 0;
            }
            
            pit_delay_ms(50);
        } else if (game_state == MEMORY_PAUSED) {
            if (needs_redraw) {
                memory_draw_pause_screen();
                needs_redraw = 0;
            }
            pit_delay_ms(50);
        } else if (game_state == MEMORY_WON) {
            memory_draw_win_screen();
            pit_delay_ms(50);
        }
    }
    
    mouse_hide_cursor();
}

void memory_game_cleanup(void) {
    vga_set_auto_swap(1);
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


void memory_draw_game(void) {
    vga_set_auto_swap(0);
    vga_clear();
    
    memory_draw_ui();

    for (int i = 0; i < total_cards; i++) {
        int row = i / grid_cols;
        int col = i % grid_cols;
        
        int x = grid_offset_x + col * (CARD_WIDTH + 1);
        int y = grid_offset_y + row * (CARD_HEIGHT + 1);
        
        int is_selected = (i == cursor_pos);
        memory_draw_card(x, y, &cards[i], is_selected);
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void memory_draw_menu(void) {
    vga_set_auto_swap(0);
    vga_clear();

    vga_set_color(VGA_LMAGENTA, VGA_BLCK);
    const char* title[] = {
        " __  __ _____ __  __  ___  ______   __",
        "|  \\/  | ____|  \\/  |/ _ \\|  _ \\ \\ / /",
        "| |\\/| |  _| | |\\/| | | | | |_) \\ V / ",
        "| |  | | |___| |  | | |_| |  _ < | |  ",
        "|_|  |_|_____|_|  |_|\\___/|_| \\_\\|_|  "
    };
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        uint8_t colors[] = {VGA_LMAGENTA, VGA_MAGENTA, VGA_LRED, VGA_RED, VGA_YELLOW};
        vga_set_color(colors[i], VGA_BLCK);
        
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 3 + i, title[i][j]);
        }
    }

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* subtitle = "Match all the pairs!";
    int len = 0;
    while (subtitle[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 9, subtitle[i]);
    }

    const char* options[] = {
        "Play Game",
        "Easy (4x3 - 6 pairs)",
        "Medium (4x4 - 8 pairs)",
        "Hard (6x4 - 12 pairs)"
    };
    
    for (int i = 0; i < 4; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_fill_rect(18, 12 + i * 2, 44, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 20;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, options[i][j]);
        }

        if (i >= 1 && i <= 3 && difficulty == i - 1) {
            vga_set_color(VGA_LGREEN, selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(x + 2, 12 + i * 2, '<');
        }
    }

    vga_draw_box_double(15, 20, 50, 5, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("*** BEST SCORES ***", 21);
    
    const char* diff_labels[] = {"Easy:", "Medium:", "Hard:"};
    for (int i = 0; i < 3; i++) {
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        int x = 18 + i * 16;
        
        for (int j = 0; diff_labels[i][j]; j++) {
            vga_putchr_at(x + j, 22, diff_labels[i][j]);
        }
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        if (stats.best_moves[i] < 9999) {
            char buf[16];
            int pos = 0;
            int temp = stats.best_moves[i];
            if (temp == 0) buf[pos++] = '0';
            else {
                char d[10]; int dc = 0;
                while (temp > 0) { d[dc++] = '0' + (temp % 10); temp /= 10; }
                for (int k = dc - 1; k >= 0; k--) buf[pos++] = d[k];
            }
            buf[pos++] = 'm';
            buf[pos] = 0;
            
            for (int j = 0; buf[j]; j++) {
                vga_putchr_at(x + j, 23, buf[j]);
            }
        } else {
            vga_putchr_at(x, 23, '-');
            vga_putchr_at(x + 1, 23, '-');
        }
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void memory_draw_pause_screen(void) {
    vga_set_auto_swap(0);
    
    vga_draw_box_double(25, 9, 30, 7, VGA_YELLOW, VGA_BLCK);
    vga_fill_rect(26, 10, 28, 5, ' ', VGA_YELLOW, VGA_DGREY);
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* pause_text = "** PAUSED **";
    int len = 0;
    while (pause_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 11, pause_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_DGREY);
    const char* inst = "Press P to continue";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, inst[i]);
    }
    
    vga_set_auto_swap(1);
    vga_swap_buffers();
}

void memory_draw_win_screen(void) {
    char buf[32];
    int pos, temp;

    vga_set_auto_swap(0);

    vga_draw_box_double(15, 7, 50, 11, VGA_LGREEN, VGA_BLCK);
    vga_fill_rect(16, 8, 48, 9, ' ', VGA_YELLOW, VGA_GREEN);

    vga_set_color(VGA_YELLOW, VGA_GREEN);
    vga_print_centered("** YOU WON! **", 9);

    vga_set_color(VGA_WHITE, VGA_GREEN);
    const char* moves_label = "Moves:";
    int x = 30;
    for (int i = 0; moves_label[i]; i++)
        vga_putchr_at(x++, 11, moves_label[i]);

    pos = 0;
    temp = stats.moves;
    if (temp == 0) buf[pos++] = '0';
    else {
        char d[10]; int dc = 0;
        while (temp) { d[dc++] = '0' + (temp % 10); temp /= 10; }
        for (int i = dc - 1; i >= 0; i--) buf[pos++] = d[i];
    }
    buf[pos] = 0;

    for (int i = 0; buf[i]; i++)
        vga_putchr_at(x + i, 11, buf[i]);

    uint32_t elapsed = stats.time_elapsed;
    int mins = elapsed / 60;
    int secs = elapsed % 60;

    buf[0] = '0' + (mins / 10);
    buf[1] = '0' + (mins % 10);
    buf[2] = ':';
    buf[3] = '0' + (secs / 10);
    buf[4] = '0' + (secs % 10);
    buf[5] = 0;

    vga_print_centered(buf, 13);

    if (stats.moves <= stats.best_moves[difficulty] ||
        stats.time_elapsed <= stats.best_time[difficulty]) {
        vga_set_color(VGA_LCYAN, VGA_GREEN);
        vga_print_centered("NEW RECORD!", 15);
    }

    vga_set_auto_swap(1);
    vga_swap_buffers();
}
