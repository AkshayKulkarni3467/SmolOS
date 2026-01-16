#include "sos_pong.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_PONG_SAVE_FILE "PONGSAVE.DAT"


static Paddle player1;
static Paddle player2;
static Ball ball;

static PongState game_state;
static GameMode game_mode;
static Difficulty_Pong difficulty;
static int menu_selection;
static int difficulty_selection;
static int high_score_p1;
static int high_score_p2;
static uint32_t last_update_time;
static uint32_t ball_update_interval;
static uint32_t paddle_update_interval;
static uint32_t last_ball_update;
static uint32_t last_paddle_update;


static uint32_t pong_rand_seed = 98765;

static uint32_t pong_rand(void) {
    pong_rand_seed = (pong_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return pong_rand_seed;
}

static float pong_randf(void) {
    return (float)(pong_rand() % 1000) / 1000.0f;
}


void pong_game_init(void) {
    game_state = PONG_MENU;
    menu_selection = 0;
    difficulty_selection = 1; 
    high_score_p1 = 0;
    high_score_p2 = 0;
    game_mode = MODE_SINGLE_PLAYER;
    difficulty = DIFFICULTY_PONG_MEDIUM;
    pong_load_high_score();
}

void pong_reset_game(void) {
    player1.x = GAME_OFFSET_X + 2;
    player1.y = GAME_OFFSET_Y + (GAME_HEIGHT / 2) - (PADDLE_HEIGHT / 2);
    player1.height = PADDLE_HEIGHT;
    player1.color = VGA_LCYAN;
    player1.score = 0;
    
    player2.x = GAME_OFFSET_X + GAME_WIDTH - 3;
    player2.y = GAME_OFFSET_Y + (GAME_HEIGHT / 2) - (PADDLE_HEIGHT / 2);
    player2.height = PADDLE_HEIGHT;
    player2.color = VGA_LRED;
    player2.score = 0;
    
    ball.x = GAME_OFFSET_X + (GAME_WIDTH / 2);
    ball.y = GAME_OFFSET_Y + (GAME_HEIGHT / 2);
    ball.color = VGA_YELLOW;
    ball.speed = 100;
    
    last_update_time = pit_get_total_milliseconds();
    ball_update_interval = 30;  
    paddle_update_interval = 30;
    last_ball_update = last_update_time;
    last_paddle_update = last_update_time;
    
    pong_reset_ball(1);
}

void pong_reset_ball(int direction) {
    ball.x = GAME_OFFSET_X + (GAME_WIDTH / 2);
    ball.y = GAME_OFFSET_Y + (GAME_HEIGHT / 2);
    
    float angle = (pong_randf() - 0.5f) * 1.5f;
    float base_speed = 0.5f;  
    
    ball.vx = direction * base_speed;
    ball.vy = angle * base_speed;
    
    pit_delay_ms(500);
    last_ball_update = pit_get_total_milliseconds();
}


void pong_update_game(void) {
    uint32_t current_time = pit_get_total_milliseconds();
    
    if (current_time - last_paddle_update >= paddle_update_interval) {
        pong_update_paddles();
        last_paddle_update = current_time;
    }
    
    if (current_time - last_ball_update >= ball_update_interval) {
        pong_update_ball();
        last_ball_update = current_time;
    }
}

void pong_update_ball(void) {
    ball.x += ball.vx;
    ball.y += ball.vy;
    
    pong_check_collisions();
    pong_check_scoring();
}

void pong_update_paddles(void) {
    
    if (game_mode == MODE_SINGLE_PLAYER) {
        pong_ai_move();
    }
}

void pong_ai_move(void) {
    int ai_reaction = 0;
    
    switch (difficulty) {
        case DIFFICULTY_PONG_EASY:
            ai_reaction = AI_DIFFICULTY_EASY;
            break;
        case DIFFICULTY_PONG_MEDIUM:
            ai_reaction = AI_DIFFICULTY_MEDIUM;
            break;
        case DIFFICULTY_PONG_HARD:
            ai_reaction = AI_DIFFICULTY_HARD;
            break;
    }
    
    if ((pong_rand() % 100) < ai_reaction) {
        int paddle_center = player2.y + (player2.height / 2);
        int ball_pos = (int)ball.y;
        
        if (ball_pos < paddle_center - 1) {
            player2.y--;
        } else if (ball_pos > paddle_center + 1) {
            player2.y++;
        }
        
        if (player2.y < GAME_OFFSET_Y) player2.y = GAME_OFFSET_Y;
        if (player2.y + player2.height >= GAME_OFFSET_Y + GAME_HEIGHT) {
            player2.y = GAME_OFFSET_Y + GAME_HEIGHT - player2.height;
        }
    }
}

void pong_check_collisions(void) {
    if (ball.y <= GAME_OFFSET_Y) {
        ball.y = GAME_OFFSET_Y;
        ball.vy = -ball.vy;
    }
    if (ball.y >= GAME_OFFSET_Y + GAME_HEIGHT - 1) {
        ball.y = GAME_OFFSET_Y + GAME_HEIGHT - 1;
        ball.vy = -ball.vy;
    }
    
    if (ball.vx < 0 && 
        (int)ball.x <= player1.x + 1 && 
        (int)ball.x >= player1.x &&
        (int)ball.y >= player1.y && 
        (int)ball.y < player1.y + player1.height) {
        
        ball.vx = -ball.vx;
        ball.x = player1.x + 1;
        
        float hit_pos = ((ball.y - player1.y) / (float)player1.height) - 0.5f;
        ball.vy += hit_pos * 0.3f;
        
        ball.vx *= 1.05f;
        ball.vy *= 1.05f;
    }
    
    if (ball.vx > 0 && 
        (int)ball.x >= player2.x - 1 && 
        (int)ball.x <= player2.x &&
        (int)ball.y >= player2.y && 
        (int)ball.y < player2.y + player2.height) {
        
        ball.vx = -ball.vx;
        ball.x = player2.x - 1;
        
        float hit_pos = ((ball.y - player2.y) / (float)player2.height) - 0.5f;
        ball.vy += hit_pos * 0.3f;
        
        ball.vx *= 1.05f;
        ball.vy *= 1.05f;
    }
}

void pong_check_scoring(void) {
    if (ball.x <= GAME_OFFSET_X) {
        player2.score++;
        
        if (player2.score >= MAX_SCORE) {
            game_state = PONG_GAME_OVER;
            if (player2.score > high_score_p2) {
                high_score_p2 = player2.score;
                pong_save_high_score();
            }
        } else {
            pong_reset_ball(1);
        }
    }
    
    if (ball.x >= GAME_OFFSET_X + GAME_WIDTH - 1) {
        player1.score++;
        
        if (player1.score >= MAX_SCORE) {
            game_state = PONG_GAME_OVER;
            if (player1.score > high_score_p1) {
                high_score_p1 = player1.score;
                pong_save_high_score();
            }
        } else {
            pong_reset_ball(-1);
        }
    }
}


void pong_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == PONG_MENU) {
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 2;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 2) menu_selection = 0;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                game_mode = MODE_SINGLE_PLAYER;
                game_state = PONG_DIFFICULTY_SELECT;
            } else if (menu_selection == 1) {
                game_mode = MODE_TWO_PLAYER;
                pong_reset_game();
                game_state = PONG_PLAYING;
            } else if (menu_selection == 2) {
                game_state = PONG_EXIT;
            }
        } else if (c == 27) {  
            game_state = PONG_EXIT;
        }
    } else if (game_state == PONG_DIFFICULTY_SELECT) {
        if (c == 0x11) {  
            difficulty_selection--;
            if (difficulty_selection < 0) difficulty_selection = 3;
        } else if (c == 0x12) {  
            difficulty_selection++;
            if (difficulty_selection > 3) difficulty_selection = 0;
        } else if (c == '\n') {  
            if (difficulty_selection < 3) {
                difficulty = (Difficulty_Pong)difficulty_selection;
                pong_reset_game();
                game_state = PONG_PLAYING;
            } else {
                game_state = PONG_MENU;
            }
        } else if (c == 27) {  
            game_state = PONG_MENU;
        }
    } else if (game_state == PONG_PLAYING) {
        if (c == 'w' || c == 'W') {  
            player1.y--;
            if (player1.y < GAME_OFFSET_Y) player1.y = GAME_OFFSET_Y;
        } else if (c == 's' || c == 'S') {  
            player1.y++;
            if (player1.y + player1.height >= GAME_OFFSET_Y + GAME_HEIGHT) {
                player1.y = GAME_OFFSET_Y + GAME_HEIGHT - player1.height;
            }
        }
        
        if (game_mode == MODE_TWO_PLAYER) {
            if (c == 0x11) {  
                player2.y--;
                if (player2.y < GAME_OFFSET_Y) player2.y = GAME_OFFSET_Y;
            } else if (c == 0x12) {  
                player2.y++;
                if (player2.y + player2.height >= GAME_OFFSET_Y + GAME_HEIGHT) {
                    player2.y = GAME_OFFSET_Y + GAME_HEIGHT - player2.height;
                }
            }
        } else {
            if (c == 0x11) {  
                player1.y--;
                if (player1.y < GAME_OFFSET_Y) player1.y = GAME_OFFSET_Y;
            } else if (c == 0x12) {  
                player1.y++;
                if (player1.y + player1.height >= GAME_OFFSET_Y + GAME_HEIGHT) {
                    player1.y = GAME_OFFSET_Y + GAME_HEIGHT - player1.height;
                }
            }
        }
        
        if (c == 'p' || c == 'P') {
            game_state = PONG_PAUSED;
        } else if (c == 27) {  
            game_state = PONG_MENU;
        }
    } else if (game_state == PONG_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = PONG_PLAYING;
            last_ball_update = pit_get_total_milliseconds();
            last_paddle_update = pit_get_total_milliseconds();
        } else if (c == 27) {  
            game_state = PONG_MENU;
        }
    }
}


void pong_draw_menu(void) {
    vga_clear();
    
    const char* title[] = {
        " ____   ___  _   _  ____ ",
        "|  _ \\ / _ \\| \\ | |/ ___|",
        "| |_) | | | |  \\| | |  _ ",
        "|  __/| |_| | |\\  | |_| |",
        "|_|    \\___/|_| \\_|\\____|"
    };
    
    uint8_t colors[] = {VGA_LGREEN, VGA_LCYAN, VGA_YELLOW, VGA_LRED, VGA_MAGENTA};
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        vga_set_color(colors[i], VGA_BLCK);
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 3 + i, title[i][j]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* subtitle = "Classic Arcade Action!";
    int len = 0;
    while (subtitle[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 9, subtitle[i]);
    }
    
    const char* options[] = {
        "Single Player",
        "Two Players",
        "Exit"
    };
    
    for (int i = 0; i < 3; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(25, 12 + i * 2, 30, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 32;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, options[i][j]);
        }
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char hs_text[50];
    int pos = 0;
    const char* hs_label = "Best Score: ";
    for (int i = 0; hs_label[i]; i++) hs_text[pos++] = hs_label[i];
    
    int temp = high_score_p1 > high_score_p2 ? high_score_p1 : high_score_p2;
    if (temp == 0) {
        hs_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            hs_text[pos++] = digits[i];
        }
    }
    hs_text[pos] = '\0';
    
    len = 0;
    while (hs_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 19, hs_text[i]);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to exit";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
}

void pong_draw_difficulty_menu(void) {
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* title = "SELECT DIFFICULTY";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 5, title[i]);
    }
    
    const char* difficulties[] = {
        "Easy   - Slow AI",
        "Medium - Normal AI",
        "Hard   - Fast AI",
        "Back to Menu"
    };
    
    const char* descriptions[] = {
        "Perfect for beginners",
        "A fair challenge",
        "Test your reflexes!",
        ""
    };
    
    for (int i = 0; i < 4; i++) {
        int selected = (i == difficulty_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(20, 10 + i * 3, 40, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 28;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 10 + i * 3, prefix[j]);
        }
        
        for (int j = 0; difficulties[i][j]; j++) {
            vga_putchr_at(x++, 10 + i * 3, difficulties[i][j]);
        }
        
        if (i < 3 && !selected) {
            vga_set_color(VGA_DGREY, VGA_BLCK);
            len = 0;
            while (descriptions[i][len]) len++;
            for (int j = 0; j < len; j++) {
                vga_putchr_at((80 - len) / 2 + j, 11 + i * 3, descriptions[i][j]);
            }
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to go back";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
}

void pong_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    pong_draw_ui();
    pong_draw_field();
    pong_draw_paddles();
    pong_draw_ball();
    pong_draw_controls();
    
    vga_end_batch();
}

void pong_draw_field(void) {
    vga_draw_box_double(GAME_OFFSET_X, GAME_OFFSET_Y, 
                        GAME_WIDTH, GAME_HEIGHT, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    for (int y = GAME_OFFSET_Y + 1; y < GAME_OFFSET_Y + GAME_HEIGHT - 1; y += 2) {
        vga_putchr_at(GAME_OFFSET_X + GAME_WIDTH / 2, y, 0xB3);
    }
}

void pong_draw_paddles(void) {
    vga_set_color(player1.color, VGA_BLCK);
    for (int i = 0; i < player1.height; i++) {
        vga_putchr_at(player1.x, player1.y + i, 0xDB);
    }
    
    vga_set_color(player2.color, VGA_BLCK);
    for (int i = 0; i < player2.height; i++) {
        vga_putchr_at(player2.x, player2.y + i, 0xDB);
    }
}

void pong_draw_ball(void) {
    vga_set_color(ball.color, VGA_BLCK);
    vga_putchr_at((int)ball.x, (int)ball.y, 0x07);  
}

void pong_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== PONG ===", 0);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char p1_score[10];
    int pos = 0;
    int temp = player1.score;
    if (temp == 0) {
        p1_score[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            p1_score[pos++] = digits[i];
        }
    }
    p1_score[pos] = '\0';
    
    for (int i = 0; p1_score[i]; i++) {
        vga_putchr_at(15 + i, GAME_OFFSET_Y + GAME_HEIGHT / 2, p1_score[i]);
    }
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    char p2_score[10];
    pos = 0;
    temp = player2.score;
    if (temp == 0) {
        p2_score[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            p2_score[pos++] = digits[i];
        }
    }
    p2_score[pos] = '\0';
    
    for (int i = 0; p2_score[i]; i++) {
        vga_putchr_at(63 + i, GAME_OFFSET_Y + GAME_HEIGHT / 2, p2_score[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* mode_text = (game_mode == MODE_SINGLE_PLAYER) ? "vs SmolOS" : "vs P2";
    int len = 0;
    while (mode_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, GAME_OFFSET_Y + GAME_HEIGHT + 1, mode_text[i]);
    }
}

void pong_draw_controls(void) {
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls = "W/S: Move | P: Pause | ESC: Menu";
    int len = 0;
    while (controls[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 24, controls[i]);
    }
}

void pong_draw_pause_screen(void) {
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

void pong_draw_game_over(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(15, 5, 50, 13, VGA_RED, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* go_text = "GAME OVER!";
    int len = 0;
    while (go_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 7, go_text[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* winner_text;
    if (player1.score > player2.score) {
        winner_text = "PLAYER 1 WINS!";
    } else {
        winner_text = (game_mode == MODE_SINGLE_PLAYER) ? "SmolOS WINS!" : "PLAYER 2 WINS!";
    }
    
    len = 0;
    while (winner_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 9, winner_text[i]);
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char score_text[40];
    int pos = 0;
    const char* p1_label = "Player 1: ";
    for (int i = 0; p1_label[i]; i++) score_text[pos++] = p1_label[i];
    
    int temp = player1.score;
    char digits[10];
    int digit_count = 0;
    if (temp == 0) {
        score_text[pos++] = '0';
    } else {
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
        vga_putchr_at(40 - len / 2 + i, 11, score_text[i]);
    }
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    pos = 0;
    const char* p2_label = (game_mode == MODE_SINGLE_PLAYER) ? "SmolOS: " : "Player 2: ";
    for (int i = 0; p2_label[i]; i++) score_text[pos++] = p2_label[i];
    
    temp = player2.score;
    digit_count = 0;
    if (temp == 0) {
        score_text[pos++] = '0';
    } else {
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
        vga_putchr_at(40 - len / 2 + i, 13, score_text[i]);
    }
    
    if ((player1.score > high_score_p1 && player1.score > player2.score) ||
        (player2.score > high_score_p2 && player2.score > player1.score)) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        const char* new_hs = "*** NEW HIGH SCORE! ***";
        len = 0;
        while (new_hs[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 15, new_hs[i]);
        }
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst = "Press any key to return to menu";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 17, inst[i]);
    }
    
    vga_end_batch();
    
    wait_for_char();
}

void pong_game_run(void) {
    pong_game_init();
    
    while (1) {
        pong_handle_input();
        
        if (game_state == PONG_EXIT) {
            break;
        }
        
        if (game_state == PONG_MENU) {
            pong_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == PONG_DIFFICULTY_SELECT) {
            pong_draw_difficulty_menu();
            pit_delay_ms(50);
        } else if (game_state == PONG_PLAYING) {
            pong_update_game();
            pong_draw_game();
            pit_delay_ms(16);
        } else if (game_state == PONG_PAUSED) {
            pong_draw_game();
            pong_draw_pause_screen();
            pit_delay_ms(50);
        } else if (game_state == PONG_GAME_OVER) {
            pong_draw_game_over();
            game_state = PONG_MENU;
        }
    }
}

void pong_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


int pong_abs(int x) {
    return x < 0 ? -x : x;
}

void pong_save_high_score(void) {
    typedef struct {
        int magic_number;     
        int high_score_p1;
        int high_score_p2;
        int version;           
        int checksum;
    } GamePongSaveData;

    GamePongSaveData save_data;
    save_data.magic_number = 0x56565656;  
    save_data.high_score_p1 = high_score_p1;
    save_data.high_score_p2 = high_score_p2;
    save_data.version = 1;

    save_data.checksum = save_data.magic_number + 
                         save_data.high_score_p1 +
                         save_data.high_score_p2 +  
                         save_data.version;

    fat16_write_file(GAME_PONG_SAVE_FILE, 
        (const char*)&save_data, 
        sizeof(GamePongSaveData));
}

void pong_load_high_score(void) {
    if (!fat16_file_exists(GAME_PONG_SAVE_FILE)) {
        high_score_p1 = 0;
        high_score_p2 = 0;
        return;
    }

    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_PONG_SAVE_FILE, &file_size);

    if (!file_content || file_size == 0) {
        high_score_p1 = 0;
        high_score_p2 = 0;
        return;
    }

    typedef struct {
        int magic_number;     
        int high_score_p1;
        int high_score_p2;
        int version;           
        int checksum;
    } GamePongSaveData;

    if (file_size < sizeof(GamePongSaveData)) {
        high_score_p1 = 0;
        high_score_p2 = 0;
        return;
    }

    GamePongSaveData* save_data = (GamePongSaveData*)file_content;

    if (save_data->magic_number != 0x56565656) {
        high_score_p1 = 0;
        high_score_p2 = 0;
        return;
    }

    int calculated_checksum = save_data->magic_number + 
                             save_data->high_score_p1 + 
                             save_data->high_score_p2 + 
                             save_data->version;

    if (calculated_checksum != save_data->checksum) {
        high_score_p1 = 0;
        high_score_p2 = 0;
        return;
    }

    high_score_p1 = save_data->high_score_p1;
    high_score_p2 = save_data->high_score_p2;
}