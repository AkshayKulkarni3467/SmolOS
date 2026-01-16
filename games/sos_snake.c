#include "sos_snake.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_SNAKE_SAVE_FILE "SNAKESAVE.DAT"


static SnakeSegment snake[SNAKE_MAX_LENGTH];
static int snake_length;
static Direction current_direction;
static Direction next_direction;
static GameState game_state;
static Difficulty difficulty;

static Food current_food;
static int score;
static int high_score;
static int game_speed;
static int menu_selection;


static uint32_t snake_rand_seed = 12345;

static uint32_t snake_rand(void) {
    snake_rand_seed = (snake_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return snake_rand_seed;
}


void snake_game_init(void) {
    game_state = GAME_MENU;
    difficulty = DIFFICULTY_MEDIUM;
    menu_selection = 0;
    high_score = 0;
    snake_load_high_score();
}


static void snake_reset_game(void) {
    snake_length = 5;
    int start_x = GAME_WIDTH / 2;
    int start_y = GAME_HEIGHT / 2;
    
    for (int i = 0; i < snake_length; i++) {
        snake[i].x = start_x - i;
        snake[i].y = start_y;
    }
    
    current_direction = DIR_RIGHT;
    next_direction = DIR_RIGHT;
    score = 0;
    game_speed = snake_get_speed(difficulty);
    
    snake_spawn_food();
}

void snake_spawn_food(void) {
    int valid = 0;
    
    while (!valid) {
        current_food.x = (snake_rand() % (GAME_WIDTH - 2)) + 1;
        current_food.y = (snake_rand() % (GAME_HEIGHT - 2)) + 1;
        
        valid = 1;
        for (int i = 0; i < snake_length; i++) {
            if (snake[i].x == current_food.x && snake[i].y == current_food.y) {
                valid = 0;
                break;
            }
        }
    }
    
    int food_type = snake_rand() % 10;
    if (food_type < 6) {
        current_food.symbol = 0x04;  
        current_food.color = VGA_LRED;
        current_food.points = 10;
    } else if (food_type < 8) {
        current_food.symbol = 0x0F;  
        current_food.color = VGA_YELLOW;
        current_food.points = 25;
    } else {
        current_food.symbol = 0x0E;  
        current_food.color = VGA_LCYAN;
        current_food.points = 50;
    }
}

void snake_grow(void) {
    if (snake_length < SNAKE_MAX_LENGTH) {
        snake[snake_length].x = snake[snake_length - 1].x;
        snake[snake_length].y = snake[snake_length - 1].y;
        snake_length++;
    }
}

void snake_update(void) {
    if (next_direction == DIR_UP && current_direction != DIR_DOWN) {
        current_direction = next_direction;
    } else if (next_direction == DIR_DOWN && current_direction != DIR_UP) {
        current_direction = next_direction;
    } else if (next_direction == DIR_LEFT && current_direction != DIR_RIGHT) {
        current_direction = next_direction;
    } else if (next_direction == DIR_RIGHT && current_direction != DIR_LEFT) {
        current_direction = next_direction;
    }
    
    for (int i = snake_length - 1; i > 0; i--) {
        snake[i].x = snake[i - 1].x;
        snake[i].y = snake[i - 1].y;
    }
    
    switch (current_direction) {
        case DIR_UP:    snake[0].y--; break;
        case DIR_DOWN:  snake[0].y++; break;
        case DIR_LEFT:  snake[0].x--; break;
        case DIR_RIGHT: snake[0].x++; break;
    }
    
    if (snake[0].x == current_food.x && snake[0].y == current_food.y) {
        score += current_food.points;
        snake_grow();
        snake_spawn_food();
        
        if (score % 100 == 0 && game_speed > 30) {
            game_speed -= 5;
        }
    }
}

int snake_check_collision(void) {
    if (snake[0].x <= 0 || snake[0].x >= GAME_WIDTH - 1 ||
        snake[0].y <= 0 || snake[0].y >= GAME_HEIGHT - 1) {
        return 1;
    }
    
    for (int i = 1; i < snake_length; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
            return 1;
        }
    }
    
    return 0;
}

void snake_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == GAME_MENU) {
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 4;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 4) menu_selection = 0;
        } else if (c == '\n') { 
            if (menu_selection == 0) {
                snake_reset_game();
                game_state = GAME_PLAYING;
            } else if (menu_selection >= 1 && menu_selection <= 4) {
                difficulty = menu_selection - 1;
            }
        } else if (c == 27) {  
            game_state = GAME_OVER;
        }
    } else if (game_state == GAME_PLAYING) {
        if (c == 0x11) {  
            next_direction = DIR_UP;
        } else if (c == 0x12) {  
            next_direction = DIR_DOWN;
        } else if (c == 0x13) {  
            next_direction = DIR_LEFT;
        } else if (c == 0x14) {  
            next_direction = DIR_RIGHT;
        } else if (c == 'p' || c == 'P' || c == ' ') {
            game_state = GAME_PAUSED;
        } else if (c == 27) {  
            game_state = GAME_MENU;
        }
    } else if (game_state == GAME_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = GAME_PLAYING;
        } else if (c == 27) {  
            game_state = GAME_MENU;
        }
    }
}


void snake_draw_border(void) {
    vga_draw_box_double(GAME_OFFSET_X, GAME_OFFSET_Y, GAME_WIDTH, GAME_HEIGHT, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(GAME_OFFSET_X - 1, GAME_OFFSET_Y - 1, 0x0C);  
    vga_putchr_at(GAME_OFFSET_X + GAME_WIDTH, GAME_OFFSET_Y - 1, 0x0C);  
    vga_putchr_at(GAME_OFFSET_X - 1, GAME_OFFSET_Y + GAME_HEIGHT, 0x0C);  
    vga_putchr_at(GAME_OFFSET_X + GAME_WIDTH, GAME_OFFSET_Y + GAME_HEIGHT, 0x0C);  
}

void snake_draw_snake(void) {
    int head_x = GAME_OFFSET_X + snake[0].x;
    int head_y = GAME_OFFSET_Y + snake[0].y;
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    char head_char;
    switch (current_direction) {
        case DIR_UP:    head_char = '^'; break;
        case DIR_DOWN:  head_char = 'v'; break;
        case DIR_LEFT:  head_char = '<'; break;
        case DIR_RIGHT: head_char = '>'; break;
        default:        head_char = 'O'; break;
    }
    vga_putchr_at(head_x, head_y, head_char);
    
    for (int i = 1; i < snake_length; i++) {
        int body_x = GAME_OFFSET_X + snake[i].x;
        int body_y = GAME_OFFSET_Y + snake[i].y;
        
        uint8_t color;
        if (i < snake_length / 3) {
            color = VGA_LGREEN;
        } else if (i < (2 * snake_length) / 3) {
            color = VGA_GREEN;
        } else {
            color = VGA_DGREY;
        }
        
        vga_set_color(color, VGA_BLCK);
        vga_putchr_at(body_x, body_y, 0xDB);  
    }
}

void snake_draw_food(void) {
    int food_x = GAME_OFFSET_X + current_food.x;
    int food_y = GAME_OFFSET_Y + current_food.y;
    
    vga_set_color(current_food.color, VGA_BLCK);
    vga_putchr_at(food_x, food_y, current_food.symbol);
}

void snake_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== SNAKE GAME ===", 0);
    
    vga_draw_box_single(0, 23, 40, 2, VGA_YELLOW, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    char score_str[20];
    int pos = 0;
    score_str[pos++] = 'S';
    score_str[pos++] = 'c';
    score_str[pos++] = 'o';
    score_str[pos++] = 'r';
    score_str[pos++] = 'e';
    score_str[pos++] = ':';
    score_str[pos++] = ' ';
    
    int temp = score;
    if (temp == 0) {
        score_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            score_str[pos++] = digits[i];
        }
    }
    score_str[pos] = '\0';
    
    for (int i = 0; score_str[i]; i++) {
        vga_putchr_at(2 + i, 23, score_str[i]);
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    pos = 0;
    char high_str[20];
    high_str[pos++] = 'H';
    high_str[pos++] = 'i';
    high_str[pos++] = 'g';
    high_str[pos++] = 'h';
    high_str[pos++] = ':';
    high_str[pos++] = ' ';
    
    temp = high_score;
    if (temp == 0) {
        high_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            high_str[pos++] = digits[i];
        }
    }
    high_str[pos] = '\0';
    
    for (int i = 0; high_str[i]; i++) {
        vga_putchr_at(2 + i, 24, high_str[i]);
    }
    
    vga_draw_box_single(40, 23, 40, 2, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_LGREY, VGA_BLCK);
    
    const char* info1 = "P:Pause  ESC:Menu";
    const char* info2 = "Arrows: Move";
    
    for (int i = 0; info1[i]; i++) {
        vga_putchr_at(42 + i, 23, info1[i]);
    }
    for (int i = 0; info2[i]; i++) {
        vga_putchr_at(42 + i, 24, info2[i]);
    }
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    char len_str[20];
    pos = 0;
    len_str[pos++] = 'L';
    len_str[pos++] = 'e';
    len_str[pos++] = 'n';
    len_str[pos++] = ':';
    len_str[pos++] = ' ';
    
    temp = snake_length;
    if (temp == 0) {
        len_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            len_str[pos++] = digits[i];
        }
    }
    len_str[pos] = '\0';
    
    for (int i = 0; len_str[i]; i++) {
        vga_putchr_at(60 + i, 23, len_str[i]);
    }
}

void snake_draw_menu(void) {
    vga_clear();
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* title[] = {
        "  _____ _   _          _  ________ ",
        " / ____| \\ | |   /\\   | |/ /  ____|",
        "| (___ |  \\| |  /  \\  | ' /| |__   ",
        " \\___ \\| . ` | / /\\ \\ |  < |  __|  ",
        " ____) | |\\  |/ ____ \\| . \\| |____ ",
        "|_____/|_| \\_/_/    \\_\\_|\\_\\______|"
    };
    
    for (int i = 0; i < 6; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        uint8_t colors[] = {VGA_LGREEN, VGA_GREEN, VGA_YELLOW, VGA_LRED, VGA_RED, VGA_MAGENTA};
        vga_set_color(colors[i], VGA_BLCK);
        
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 3 + i, title[i][j]);
        }
    }
    
    const char* options[] = {
        "Play Game",
        "Easy",
        "Medium",
        "Hard",
        "Expert"
    };
    
    for (int i = 0; i < 5; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(25, 12 + i * 2, 30, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 27;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, options[i][j]);
        }
        
        if (i >= 1 && i <= 4 && difficulty == i - 1) {
            vga_set_color(VGA_LGREEN, selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(x + 2, 12 + i * 2, '<');
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to exit";
    int len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char hs[30];
    int pos = 0;
    const char* hs_text = "High Score: ";
    for (int i = 0; hs_text[i]; i++) hs[pos++] = hs_text[i];
    
    int temp = high_score;
    if (temp == 0) {
        hs[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            hs[pos++] = digits[i];
        }
    }
    hs[pos] = '\0';
    
    len = 0;
    while (hs[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 10, hs[i]);
    }
}

void snake_draw_pause_screen(void) {
    vga_draw_box_double(25, 10, 30, 5, VGA_YELLOW, VGA_BLCK);
    vga_fill_rect(26, 11, 28, 3, ' ', VGA_YELLOW, VGA_DGREY);
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* pause_text = "PAUSED";
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
}

void snake_draw_game_over(void) {
    vga_clear();
    
    vga_draw_box_double(15, 5, 50, 14, VGA_RED, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* go_text = "GAME OVER!";
    int len = 0;
    while (go_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 7, go_text[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_text[30];
    int pos = 0;
    const char* prefix = "Final Score: ";
    for (int i = 0; prefix[i]; i++) score_text[pos++] = prefix[i];
    
    int temp = score;
    if (temp == 0) {
        score_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            score_text[pos++] = digits[i];
        }
    }
    score_text[pos] = '\0';
    
    len = 0;
    while (score_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, score_text[i]);
    }
    
    if (score > high_score) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        const char* new_hs = "NEW HIGH SCORE!";
        len = 0;
        while (new_hs[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 12, new_hs[i]);
        }
        high_score = score;
        snake_save_high_score();
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char length_text[30];
    pos = 0;
    const char* len_prefix = "Length: ";
    for (int i = 0; len_prefix[i]; i++) length_text[pos++] = len_prefix[i];
    
    temp = snake_length;
    if (temp == 0) {
        length_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            length_text[pos++] = digits[i];
        }
    }
    length_text[pos] = '\0';
    
    len = 0;
    while (length_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 14, length_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst = "Press any key to return to menu";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 17, inst[i]);
    }
    
    wait_for_char();
    game_state = GAME_MENU;
}

void snake_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    snake_draw_ui();
    snake_draw_border();
    snake_draw_food();
    snake_draw_snake();
    
    vga_end_batch();
}

int snake_get_speed(Difficulty diff) {
    switch (diff) {
        case DIFFICULTY_EASY:   return 150;
        case DIFFICULTY_MEDIUM: return 100;
        case DIFFICULTY_HARD:   return 70;
        case DIFFICULTY_EXPERT: return 50;
        default:                return 100;
    }
}

void snake_save_high_score(void) {
    typedef struct {
        int magic_number;     
        int high_score;
        int version;           
        int checksum;
    } GameSnakeSaveData;
    
    GameSnakeSaveData save_data;
    save_data.magic_number = 0x67676767;  
    save_data.high_score = high_score;
    save_data.version = 1;
    
    save_data.checksum = save_data.magic_number + 
                         save_data.high_score + 
                         save_data.version;
    
    fat16_write_file(GAME_SNAKE_SAVE_FILE, 
                     (const char*)&save_data, 
                     sizeof(GameSnakeSaveData));
}

void snake_load_high_score(void) {
    if (!fat16_file_exists(GAME_SNAKE_SAVE_FILE)) {
        high_score = 0;
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_SNAKE_SAVE_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        high_score = 0;
        return;
    }
    
    typedef struct {
        int magic_number;
        int high_score;
        int version;
        int checksum;
    } GameSnakeSaveData;
    
    if (file_size < sizeof(GameSnakeSaveData)) {
        high_score = 0;
        return;
    }
    
    GameSnakeSaveData* save_data = (GameSnakeSaveData*)file_content;
    
    if (save_data->magic_number != 0x67676767) {
        high_score = 0;
        return;
    }
    
    int calculated_checksum = save_data->magic_number + 
                             save_data->high_score + 
                             save_data->version;
    
    if (calculated_checksum != save_data->checksum) {
        high_score = 0;
        return;
    }
    
    high_score = save_data->high_score;
}

void snake_game_run(void) {
    snake_game_init();
    
    uint32_t last_update = 0;
    
    while (game_state != GAME_OVER || game_state == GAME_MENU) {
        snake_handle_input();
        
        if (game_state == GAME_MENU) {
            snake_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == GAME_PLAYING) {
            uint32_t current_time = pit_get_milliseconds();
            
            if (current_time - last_update >= game_speed) {
                snake_update();
                
                if (snake_check_collision()) {
                    snake_draw_game_over();
                }
                
                last_update = current_time;
            }
            
            snake_draw_game();
            pit_delay_ms(16);  
        } else if (game_state == GAME_PAUSED) {
            snake_draw_pause_screen();
            pit_delay_ms(50);
        }
        
        if (game_state == GAME_MENU && menu_selection == 0) {

        }
    }
}

void snake_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}