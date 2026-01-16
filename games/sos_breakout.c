#include "sos_breakout.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_BREAKOUT_SAVE_FILE "BREAKOUTSAVE.DAT"



static Brick bricks[BRICK_ROWS][BRICK_COLS];
static Breakout_Paddle paddle;
static Breakout_Ball balls[5];  
static Powerup powerups[10];
static Laser lasers[10];

static BreakoutState game_state;
static int menu_selection;
static int level_selection;
static int current_level;
static int score;
static int high_score;
static int lives;
static int bricks_remaining;

static uint32_t last_update_time;
static uint32_t powerup_end_time;
static int paddle_expand_active;
static int ball_slow_active;


static uint32_t breakout_rand_seed = 12345;

static uint32_t breakout_rand(void) {
    breakout_rand_seed = (breakout_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return breakout_rand_seed;
}

static float breakout_randf(void) {
    return (float)(breakout_rand() % 1000) / 1000.0f;
}

static float breakout_abs(float x) {
    return x < 0 ? -x : x;
}


void breakout_game_init(void) {
    game_state = BREAKOUT_MENU;
    menu_selection = 0;
    level_selection = 0;
    current_level = 1;
    high_score = 0;
    breakout_load_high_score();
}

void breakout_reset_game(void) {
    score = 0;
    lives = 3;
    current_level = 1;
    paddle_expand_active = 0;
    ball_slow_active = 0;
    powerup_end_time = 0;
    
    breakout_start_level(current_level);
}

void breakout_start_level(int level) {
    current_level = level;
    bricks_remaining = 0;
    

    paddle.x = GAME_OFFSET_X + (GAME_WIDTH / 2) - (PADDLE_WIDTH / 2);
    paddle.y = GAME_OFFSET_Y + GAME_HEIGHT - 3;
    paddle.width = PADDLE_WIDTH;
    paddle.color = VGA_LCYAN;
    paddle.laser_active = 0;
    

    for (int i = 0; i < 5; i++) {
        balls[i].active = 0;
    }
    balls[0].active = 1;
    breakout_reset_ball();

    for (int i = 0; i < 10; i++) {
        powerups[i].active = 0;
    }
    

    for (int i = 0; i < 10; i++) {
        lasers[i].active = 0;
    }
    
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            BrickType type = BRICK_NONE;
            
            if (level == 1) {
                if (row == 0) type = BRICK_RED;
                else if (row == 1) type = BRICK_ORANGE;
                else if (row == 2) type = BRICK_YELLOW;
                else if (row == 3) type = BRICK_GREEN;
                else if (row == 4) type = BRICK_BLUE;
                else if (row == 5) type = BRICK_BLUE;
            } else if (level == 2) {
                if ((row + col) % 2 == 0) {
                    type = (row < 4) ? BRICK_YELLOW : BRICK_GREEN;
                } else {
                    type = BRICK_SILVER;
                }
            } else if (level == 3) {
                if (row < 2) type = BRICK_RED;
                else if (row < 4) type = BRICK_SILVER;
                else if (row < 6) type = BRICK_YELLOW;
                else type = BRICK_BLUE;
                
                if (col % 3 == 0 && row % 2 == 0) type = BRICK_GOLD;
            } else {
                int rand_val = breakout_rand() % 10;
                if (rand_val < 2) type = BRICK_RED;
                else if (rand_val < 4) type = BRICK_ORANGE;
                else if (rand_val < 6) type = BRICK_YELLOW;
                else if (rand_val < 8) type = BRICK_SILVER;
                else if (rand_val < 9) type = BRICK_GREEN;
                else type = BRICK_GOLD;
            }
            
            bricks[row][col].type = type;
            bricks[row][col].visible = (type != BRICK_NONE);
            bricks[row][col].color = breakout_get_brick_color(type);
            bricks[row][col].hits_remaining = (type == BRICK_SILVER) ? 2 : 1;
            
            if (type != BRICK_NONE && type != BRICK_GOLD) {
                bricks_remaining++;
            }
        }
    }
    
    last_update_time = pit_get_total_milliseconds();
}

void breakout_reset_ball(void) {
    balls[0].x = paddle.x + (paddle.width / 2);
    balls[0].y = paddle.y - 1;
    
    float base_speed = ball_slow_active ? 0.25f : 0.35f;
    float angle = -1.2f + (breakout_randf() * 0.8f);
    
    balls[0].vx = angle * base_speed;
    balls[0].vy = -base_speed;
    balls[0].color = VGA_YELLOW;
    balls[0].active = 1;
}

void breakout_update_game(void) {
    uint32_t current_time = pit_get_total_milliseconds();
    
    if (powerup_end_time > 0 && current_time >= powerup_end_time) {
        paddle_expand_active = 0;
        ball_slow_active = 0;
        paddle.width = PADDLE_WIDTH;
        paddle.laser_active = 0;
        powerup_end_time = 0;
    }
    
    if (paddle.laser_active && current_time >= paddle.laser_end_time) {
        paddle.laser_active = 0;
    }
    
    breakout_update_paddle();
    breakout_update_balls();
    breakout_update_powerups();
    breakout_update_lasers();
    breakout_check_powerup_collision();
    breakout_check_level_complete();
}

void breakout_update_balls(void) {
    int any_active = 0;
    
    for (int i = 0; i < 5; i++) {
        if (!balls[i].active) continue;
        
        any_active = 1;
        
        balls[i].x += balls[i].vx;
        balls[i].y += balls[i].vy;
        
        if (balls[i].x <= GAME_OFFSET_X) {
            balls[i].x = GAME_OFFSET_X;
            balls[i].vx = -balls[i].vx;
        }
        if (balls[i].x >= GAME_OFFSET_X + GAME_WIDTH - 1) {
            balls[i].x = GAME_OFFSET_X + GAME_WIDTH - 1;
            balls[i].vx = -balls[i].vx;
        }
        if (balls[i].y <= GAME_OFFSET_Y) {
            balls[i].y = GAME_OFFSET_Y;
            balls[i].vy = -balls[i].vy;
        }
        
        if (balls[i].y >= GAME_OFFSET_Y + GAME_HEIGHT) {
            balls[i].active = 0;
        }
        
        breakout_check_paddle_collision(&balls[i]);
        breakout_check_brick_collision(&balls[i]);
    }
    
    if (!any_active) {
        breakout_lose_life();
    }
}

void breakout_update_paddle(void) {
    if (paddle.x < GAME_OFFSET_X) paddle.x = GAME_OFFSET_X;
    if (paddle.x + paddle.width >= GAME_OFFSET_X + GAME_WIDTH) {
        paddle.x = GAME_OFFSET_X + GAME_WIDTH - paddle.width;
    }
}

void breakout_update_powerups(void) {
    for (int i = 0; i < 10; i++) {
        if (!powerups[i].active) continue;
        
        powerups[i].y += powerups[i].vy;
        
        if (powerups[i].y >= GAME_OFFSET_Y + GAME_HEIGHT) {
            powerups[i].active = 0;
        }
    }
}

void breakout_update_lasers(void) {
    for (int i = 0; i < 10; i++) {
        if (!lasers[i].active) continue;
        
        lasers[i].y += lasers[i].vy;
        
        int laser_grid_x = ((int)lasers[i].x - GAME_OFFSET_X - BRICK_OFFSET_X) / BRICK_WIDTH;
        int laser_grid_y = ((int)lasers[i].y - GAME_OFFSET_Y - BRICK_OFFSET_Y) / BRICK_HEIGHT;
        
        if (laser_grid_y >= 0 && laser_grid_y < BRICK_ROWS &&
            laser_grid_x >= 0 && laser_grid_x < BRICK_COLS) {
            
            if (bricks[laser_grid_y][laser_grid_x].visible &&
                bricks[laser_grid_y][laser_grid_x].type != BRICK_GOLD) {
                
                bricks[laser_grid_y][laser_grid_x].hits_remaining--;
                if (bricks[laser_grid_y][laser_grid_x].hits_remaining <= 0) {
                    bricks[laser_grid_y][laser_grid_x].visible = 0;
                    score += breakout_get_brick_points(bricks[laser_grid_y][laser_grid_x].type);
                    bricks_remaining--;
                    
                    if (breakout_rand() % 5 == 0) {
                        int brick_x = GAME_OFFSET_X + BRICK_OFFSET_X + (laser_grid_x * BRICK_WIDTH) + (BRICK_WIDTH / 2);
                        int brick_y = GAME_OFFSET_Y + BRICK_OFFSET_Y + (laser_grid_y * BRICK_HEIGHT);
                        breakout_spawn_powerup(brick_x, brick_y);
                    }
                } else {
                    bricks[laser_grid_y][laser_grid_x].color = VGA_DGREY;
                }
                
                lasers[i].active = 0;
            }
        }
        
        if (lasers[i].y <= GAME_OFFSET_Y) {
            lasers[i].active = 0;
        }
    }
}

void breakout_check_paddle_collision(Breakout_Ball* ball) {
    if (ball->vy > 0 &&
        ball->y >= paddle.y - 1 &&
        ball->y <= paddle.y + PADDLE_HEIGHT &&
        ball->x >= paddle.x &&
        ball->x < paddle.x + paddle.width) {
        
        ball->vy = -ball->vy;
        ball->y = paddle.y - 1;
        
        float hit_pos = (ball->x - paddle.x) / (float)paddle.width;
        float angle = (hit_pos - 0.5f) * 2.0f;
        ball->vx = angle * 0.3f;
    }
}

void breakout_check_brick_collision(Breakout_Ball* ball) {
    int grid_x = ((int)ball->x - GAME_OFFSET_X - BRICK_OFFSET_X) / BRICK_WIDTH;
    int grid_y = ((int)ball->y - GAME_OFFSET_Y - BRICK_OFFSET_Y) / BRICK_HEIGHT;
    
    if (grid_y < 0 || grid_y >= BRICK_ROWS || grid_x < 0 || grid_x >= BRICK_COLS) {
        return;
    }
    
    if (!bricks[grid_y][grid_x].visible) return;
    if (bricks[grid_y][grid_x].type == BRICK_GOLD) {
        ball->vy = -ball->vy;
        return;
    }
    
    bricks[grid_y][grid_x].hits_remaining--;
    
    if (bricks[grid_y][grid_x].hits_remaining <= 0) {
        bricks[grid_y][grid_x].visible = 0;
        score += breakout_get_brick_points(bricks[grid_y][grid_x].type);
        bricks_remaining--;
        
        if (breakout_rand() % 4 == 0) {
            int brick_x = GAME_OFFSET_X + BRICK_OFFSET_X + (grid_x * BRICK_WIDTH) + (BRICK_WIDTH / 2);
            int brick_y = GAME_OFFSET_Y + BRICK_OFFSET_Y + (grid_y * BRICK_HEIGHT);
            breakout_spawn_powerup(brick_x, brick_y);
        }
    } else {
        bricks[grid_y][grid_x].color = VGA_DGREY;
    }
    
    ball->vy = -ball->vy;
}

void breakout_check_powerup_collision(void) {
    for (int i = 0; i < 10; i++) {
        if (!powerups[i].active) continue;
        
        if (powerups[i].y >= paddle.y - 1 &&
            powerups[i].y <= paddle.y + PADDLE_HEIGHT &&
            powerups[i].x >= paddle.x &&
            powerups[i].x < paddle.x + paddle.width) {
            
            breakout_activate_powerup(powerups[i].type);
            powerups[i].active = 0;
        }
    }
}

void breakout_spawn_powerup(int brick_x, int brick_y) {
    for (int i = 0; i < 10; i++) {
        if (!powerups[i].active) {
            powerups[i].x = brick_x;
            powerups[i].y = brick_y;
            powerups[i].vy = 0.15f;
            powerups[i].active = 1;
            
            int rand_val = breakout_rand() % 100;
            if (rand_val < 20) powerups[i].type = POWERUP_EXPAND;
            else if (rand_val < 35) powerups[i].type = POWERUP_SLOW;
            else if (rand_val < 45) powerups[i].type = POWERUP_MULTIBALL;
            else if (rand_val < 55) powerups[i].type = POWERUP_LASER;
            else if (rand_val < 65) powerups[i].type = POWERUP_LIFE;
            else if (rand_val < 80) powerups[i].type = POWERUP_SHRINK;
            else powerups[i].type = POWERUP_FAST;
            
            switch (powerups[i].type) {
                case POWERUP_EXPAND: powerups[i].color = VGA_LGREEN; break;
                case POWERUP_SHRINK: powerups[i].color = VGA_LRED; break;
                case POWERUP_SLOW: powerups[i].color = VGA_LCYAN; break;
                case POWERUP_FAST: powerups[i].color = VGA_LRED; break;
                case POWERUP_MULTIBALL: powerups[i].color = VGA_YELLOW; break;
                case POWERUP_LASER: powerups[i].color = VGA_LMAGENTA; break;
                case POWERUP_LIFE: powerups[i].color = VGA_LGREEN; break;
                default: powerups[i].color = VGA_WHITE; break;
            }
            break;
        }
    }
}

void breakout_activate_powerup(PowerupType type) {
    uint32_t current_time = pit_get_total_milliseconds();
    
    switch (type) {
        case POWERUP_EXPAND:
            paddle.width = PADDLE_WIDTH + 5;
            paddle_expand_active = 1;
            powerup_end_time = current_time + POWERUP_DURATION;
            break;
            
        case POWERUP_SHRINK:
            paddle.width = PADDLE_WIDTH - 3;
            if (paddle.width < 4) paddle.width = 4;
            paddle_expand_active = 0;
            powerup_end_time = current_time + POWERUP_DURATION;
            break;
            
        case POWERUP_SLOW:
            for (int i = 0; i < 5; i++) {
                if (balls[i].active) {
                    balls[i].vx *= 0.7f;
                    balls[i].vy *= 0.7f;
                }
            }
            ball_slow_active = 1;
            powerup_end_time = current_time + POWERUP_DURATION;
            break;
            
        case POWERUP_FAST:
            for (int i = 0; i < 5; i++) {
                if (balls[i].active) {
                    balls[i].vx *= 1.4f;
                    balls[i].vy *= 1.4f;
                }
            }
            ball_slow_active = 0;
            powerup_end_time = current_time + POWERUP_DURATION;
            break;
            
        case POWERUP_MULTIBALL:
            for (int i = 1; i < 5; i++) {
                if (!balls[i].active) {
                    balls[i] = balls[0];
                    balls[i].vx = (breakout_randf() - 0.5f) * 0.5f;
                    balls[i].vy = -0.35f;
                    balls[i].active = 1;
                    break;
                }
            }
            break;
            
        case POWERUP_LASER:
            paddle.laser_active = 1;
            paddle.laser_end_time = current_time + POWERUP_DURATION;
            break;
            
        case POWERUP_LIFE:
            if (lives < MAX_LIVES) lives++;
            break;
            
        default:
            break;
    }
}

void breakout_fire_laser(void) {
    if (!paddle.laser_active) return;
    
    for (int i = 0; i < 10; i++) {
        if (!lasers[i].active) {
            lasers[i].x = paddle.x + (paddle.width / 2);
            lasers[i].y = paddle.y - 1;
            lasers[i].vy = -0.8f;
            lasers[i].active = 1;
            break;
        }
    }
}

void breakout_lose_life(void) {
    lives--;
    
    if (lives <= 0) {
        game_state = BREAKOUT_GAME_OVER;
        if (score > high_score) {
            high_score = score;
            breakout_save_high_score();
        }
    } else {
        for (int i = 0; i < 5; i++) {
            balls[i].active = 0;
        }
        balls[0].active = 1;
        breakout_reset_ball();
        
        paddle.width = PADDLE_WIDTH;
        paddle_expand_active = 0;
        ball_slow_active = 0;
        paddle.laser_active = 0;
        powerup_end_time = 0;
        
        pit_delay_ms(1000);
    }
}

void breakout_check_level_complete(void) {
    if (bricks_remaining <= 0) {
        game_state = BREAKOUT_LEVEL_COMPLETE;
    }
}


void breakout_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == BREAKOUT_MENU) {
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 2;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 2) menu_selection = 0;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                breakout_reset_game();
                game_state = BREAKOUT_PLAYING;
            } else if (menu_selection == 1) {
                game_state = BREAKOUT_LEVEL_SELECT;
            } else if (menu_selection == 2) {
                game_state = BREAKOUT_EXIT;
            }
        } else if (c == 27) {  
            game_state = BREAKOUT_EXIT;
        }
    } else if (game_state == BREAKOUT_LEVEL_SELECT) {
        if (c == 0x13) {  
            level_selection--;
            if (level_selection < 0) level_selection = 3;
        } else if (c == 0x14) {  
            level_selection++;
            if (level_selection > 3) level_selection = 0;
        } else if (c == '\n') {  
            if (level_selection < 3) {
                score = 0;
                lives = 3;
                current_level = level_selection + 1;
                breakout_start_level(current_level);
                game_state = BREAKOUT_PLAYING;
            } else {
                game_state = BREAKOUT_MENU;
            }
        } else if (c == 27) {  
            game_state = BREAKOUT_MENU;
        }
    } else if (game_state == BREAKOUT_PLAYING) {
        if (c == 0x13 || c == 'a' || c == 'A') {  
            paddle.x -= 2;
        } else if (c == 0x14 || c == 'd' || c == 'D') {  
            paddle.x += 2;
        } else if (c == ' ') {  
            breakout_fire_laser();
        } else if (c == 'p' || c == 'P') {
            game_state = BREAKOUT_PAUSED;
        } else if (c == 27) {  
            game_state = BREAKOUT_MENU;
        }
    } else if (game_state == BREAKOUT_PAUSED) {
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = BREAKOUT_PLAYING;
            last_update_time = pit_get_total_milliseconds();
        } else if (c == 27) {  
            game_state = BREAKOUT_MENU;
        }
    } else if (game_state == BREAKOUT_LEVEL_COMPLETE) {
        if (c == '\n' || c == ' ') {
            current_level++;
            breakout_start_level(current_level);
            game_state = BREAKOUT_PLAYING;
        } else if (c == 27) {
            game_state = BREAKOUT_MENU;
        }
    }
}

void breakout_draw_menu(void) {
    vga_clear();
    
    const char* title[] = {
        " ____  ____  _____    _    _  _____  _   _ _____ ",
        "| __ )|  _ \\| ____|  / \\  | |/ / _ \\| | | |_   _|",
        "|  _ \\| |_) |  _|   / _ \\ | ' / | | | | | | | |  ",
        "| |_) |  _ <| |___ / ___ \\| . \\ |_| | |_| | | |  ",
        "|____/|_| \\_\\_____/_/   \\_\\_|\\_\\___/ \\___/  |_|  "
    };
    
    uint8_t colors[] = {VGA_LRED, VGA_YELLOW, VGA_LGREEN, VGA_LCYAN, VGA_LBLUE};
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        vga_set_color(colors[i], VGA_BLCK);
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 2 + i, title[i][j]);
        }
    }
    
    const char* options[] = {
        "Start Game",
        "Level Select",
        "Exit"
    };
    
    for (int i = 0; i < 3; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(28, 11 + i * 2, 24, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 32;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 11 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 11 + i * 2, options[i][j]);
        }
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char hs_text[50];
    int pos = 0;
    const char* hs_label = "High Score: ";
    for (int i = 0; hs_label[i]; i++) hs_text[pos++] = hs_label[i];
    
    int temp = high_score;
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
    
    int len = 0;
    while (hs_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 18, hs_text[i]);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use UP/DOWN arrows, ENTER to select, ESC to exit";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
}

void breakout_draw_level_select(void) {
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* title = "SELECT LEVEL";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 4, title[i]);
    }
    
    const char* levels[] = {
        "Level 1 - Classic",
        "Level 2 - Checkers",
        "Level 3 - Challenge",
        "Back to Menu"
    };
    
    for (int i = 0; i < 4; i++) {
        int selected = (i == level_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(23, 9 + i * 3, 34, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 28;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 9 + i * 3, prefix[j]);
        }
        
        for (int j = 0; levels[i][j]; j++) {
            vga_putchr_at(x++, 9 + i * 3, levels[i][j]);
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use LEFT/RIGHT arrows, ENTER to select, ESC to go back";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
}

void breakout_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    breakout_draw_ui();
    breakout_draw_field();
    breakout_draw_bricks();
    breakout_draw_paddle();
    breakout_draw_balls();
    breakout_draw_powerups();
    breakout_draw_lasers();
    breakout_draw_lives();
    
    vga_end_batch();
}

void breakout_draw_field(void) {
    vga_draw_box_double(GAME_OFFSET_X, GAME_OFFSET_Y,
                        GAME_WIDTH, GAME_HEIGHT, VGA_CYAN, VGA_BLCK);
}

void breakout_draw_bricks(void) {
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            if (!bricks[row][col].visible) continue;
            
            int x = GAME_OFFSET_X + BRICK_OFFSET_X + (col * BRICK_WIDTH);
            int y = GAME_OFFSET_Y + BRICK_OFFSET_Y + (row * BRICK_HEIGHT);
            
            vga_set_color(bricks[row][col].color, VGA_BLCK);
            
            for (int i = 0; i < BRICK_WIDTH; i++) {
                vga_putchr_at(x + i, y, 0xDB);
            }
        }
    }
}

void breakout_draw_paddle(void) {
    vga_set_color(paddle.color, VGA_BLCK);
    
    for (int i = 0; i < paddle.width; i++) {
        vga_putchr_at((int)paddle.x + i, (int)paddle.y, 0xDC);
    }
    
    if (paddle.laser_active) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_putchr_at((int)paddle.x, (int)paddle.y, '<');
        vga_putchr_at((int)paddle.x + paddle.width - 1, (int)paddle.y, '>');
    }
}

void breakout_draw_balls(void) {
    for (int i = 0; i < 5; i++) {
        if (!balls[i].active) continue;
        
        vga_set_color(balls[i].color, VGA_BLCK);
        vga_putchr_at((int)balls[i].x, (int)balls[i].y, 0x07);
    }
}

void breakout_draw_powerups(void) {
    for (int i = 0; i < 10; i++) {
        if (!powerups[i].active) continue;
        
        vga_set_color(powerups[i].color, VGA_BLCK);
        
        char symbol = '?';
        switch (powerups[i].type) {
            case POWERUP_EXPAND: symbol = 'E'; break;
            case POWERUP_SHRINK: symbol = 'S'; break;
            case POWERUP_SLOW: symbol = 'L'; break;
            case POWERUP_FAST: symbol = 'F'; break;
            case POWERUP_MULTIBALL: symbol = 'M'; break;
            case POWERUP_LASER: symbol = 'Z'; break;
            case POWERUP_LIFE: symbol = '+'; break;
            default: symbol = '?'; break;
        }
        
        vga_putchr_at((int)powerups[i].x, (int)powerups[i].y, symbol);
    }
}

void breakout_draw_lasers(void) {
    vga_set_color(VGA_LRED, VGA_BLCK);
    
    for (int i = 0; i < 10; i++) {
        if (!lasers[i].active) continue;
        vga_putchr_at((int)lasers[i].x, (int)lasers[i].y, '|');
    }
}

void breakout_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== BREAKOUT ===", 0);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_text[30];
    int pos = 0;
    const char* label = "Score: ";
    for (int i = 0; label[i]; i++) score_text[pos++] = label[i];
    
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
    
    for (int i = 0; score_text[i]; i++) {
        vga_putchr_at(2 + i, GAME_OFFSET_Y + 2, score_text[i]);
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char level_text[20];
    pos = 0;
    const char* level_label = "Level: ";
    for (int i = 0; level_label[i]; i++) level_text[pos++] = level_label[i];
    level_text[pos++] = '0' + current_level;
    level_text[pos] = '\0';
    
    for (int i = 0; level_text[i]; i++) {
        vga_putchr_at(2 + i, GAME_OFFSET_Y + 4, level_text[i]);
    }
    
    if (powerup_end_time > 0) {
        vga_set_color(VGA_LMAGENTA, VGA_BLCK);
        const char* powerup_text = "PWR";
        for (int i = 0; powerup_text[i]; i++) {
            vga_putchr_at(2 + i, GAME_OFFSET_Y + 6, powerup_text[i]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls = "A/D: Move | SPACE: Fire | P: Pause";
    int len = 0;
    while (controls[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 24, controls[i]);
    }
}

void breakout_draw_lives(void) {
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* lives_text = "Lives: ";
    int x = 2;
    for (int i = 0; lives_text[i]; i++) {
        vga_putchr_at(x++, GAME_OFFSET_Y + 8, lives_text[i]);
    }
    
    for (int i = 0; i < lives; i++) {
        vga_putchr_at(x++, GAME_OFFSET_Y + 8, 0x03);  
    }
}

void breakout_draw_pause_screen(void) {
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

void breakout_draw_level_complete(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(20, 8, 40, 10, VGA_GREEN, VGA_BLCK);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* complete_text = "LEVEL COMPLETE!";
    int len = 0;
    while (complete_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, complete_text[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_text[40];
    int pos = 0;
    const char* label = "Score: ";
    for (int i = 0; label[i]; i++) score_text[pos++] = label[i];
    
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
        vga_putchr_at(40 - len / 2 + i, 12, score_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst = "Press ENTER for next level";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, inst[i]);
    }
    
    const char* esc_text = "ESC to return to menu";
    len = 0;
    while (esc_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 16, esc_text[i]);
    }
    
    vga_end_batch();
}

void breakout_draw_game_over(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(15, 6, 50, 12, VGA_RED, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* go_text = "GAME OVER!";
    int len = 0;
    while (go_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 8, go_text[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_text[40];
    int pos = 0;
    const char* label = "Final Score: ";
    for (int i = 0; label[i]; i++) score_text[pos++] = label[i];
    
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
        vga_putchr_at(40 - len / 2 + i, 11, score_text[i]);
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char level_text[40];
    pos = 0;
    const char* level_label = "Level Reached: ";
    for (int i = 0; level_label[i]; i++) level_text[pos++] = level_label[i];
    level_text[pos++] = '0' + current_level;
    level_text[pos] = '\0';
    
    len = 0;
    while (level_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, level_text[i]);
    }
    
    if (score > high_score) {
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


void breakout_game_run(void) {
    breakout_game_init();
    
    while (1) {
        breakout_handle_input();
        
        if (game_state == BREAKOUT_EXIT) {
            break;
        }
        
        if (game_state == BREAKOUT_MENU) {
            breakout_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == BREAKOUT_LEVEL_SELECT) {
            breakout_draw_level_select();
            pit_delay_ms(50);
        } else if (game_state == BREAKOUT_PLAYING) {
            breakout_update_game();
            breakout_draw_game();
            pit_delay_ms(16);
        } else if (game_state == BREAKOUT_PAUSED) {
            breakout_draw_game();
            breakout_draw_pause_screen();
            pit_delay_ms(50);
        } else if (game_state == BREAKOUT_LEVEL_COMPLETE) {
            breakout_draw_level_complete();
            pit_delay_ms(50);
        } else if (game_state == BREAKOUT_GAME_OVER) {
            breakout_draw_game_over();
            game_state = BREAKOUT_MENU;
        }
    }
}

void breakout_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


uint8_t breakout_get_brick_color(BrickType type) {
    switch (type) {
        case BRICK_RED: return VGA_LRED;
        case BRICK_ORANGE: return VGA_LBRWN;
        case BRICK_YELLOW: return VGA_YELLOW;
        case BRICK_GREEN: return VGA_LGREEN;
        case BRICK_BLUE: return VGA_LBLUE;
        case BRICK_SILVER: return VGA_LGREY;
        case BRICK_GOLD: return VGA_YELLOW;
        default: return VGA_WHITE;
    }
}

int breakout_get_brick_points(BrickType type) {
    switch (type) {
        case BRICK_RED: return 100;
        case BRICK_ORANGE: return 80;
        case BRICK_YELLOW: return 60;
        case BRICK_GREEN: return 40;
        case BRICK_BLUE: return 20;
        case BRICK_SILVER: return 50;
        case BRICK_GOLD: return 200;
        default: return 0;
    }
}

void breakout_save_high_score(void) {
    typedef struct {
        int magic_number;      
        int high_score;
        int version;           
        int checksum;
    } GameBreakoutSaveData;

    GameBreakoutSaveData save_data;
    save_data.magic_number = 0x12121212;
    save_data.high_score = high_score;
    save_data.version = 1;

    save_data.checksum = save_data.magic_number + 
                         save_data.high_score + 
                         save_data.version;

    fat16_write_file(GAME_BREAKOUT_SAVE_FILE, 
    (const char*)&save_data, 
    sizeof(GameBreakoutSaveData));

}

void breakout_load_high_score(void) {
    if (!fat16_file_exists(GAME_BREAKOUT_SAVE_FILE)) {
        high_score = 0;
        return;
    }
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_BREAKOUT_SAVE_FILE, &file_size);
    if (!file_content || file_size == 0) {
        high_score = 0;
        return;
    }
    typedef struct {
        int magic_number;      
        int high_score;
        int version;           
        int checksum;
    } GameBreakoutSaveData;

    if (file_size < sizeof(GameBreakoutSaveData)) {
        high_score = 0;
        return;
    }

    GameBreakoutSaveData* save_data = (GameBreakoutSaveData*)file_content;
    if (save_data->magic_number != 0x12121212) {
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