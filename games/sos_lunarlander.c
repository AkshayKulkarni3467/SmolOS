#include "sos_lunarlander.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_io.h" 

static Ship ship;
static Terrain terrain;
static LanderState game_state;
static LanderDifficulty difficulty;
static int menu_selection;
static int difficulty_selection;
static int exit_requested;
static int needs_redraw;

static float camera_x;
static int score;
static int bonus_multiplier;
static LandingResult landing_result;
static uint32_t game_start_time;

#define MAX_PARTICLES 100
static LunarParticle particles[MAX_PARTICLES];

static LanderStats stats;

static uint32_t lander_rand_seed = 87654;

static uint32_t lander_rand(void) {
    lander_rand_seed = (lander_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return lander_rand_seed;
}

static float lander_randf(void) {
    return (float)(lander_rand() % 1000) / 1000.0f;
}

float lander_abs(float x) {
    return x < 0 ? -x : x;
}


void lunar_lander_init(void) {
    game_state = LANDER_MENU;
    difficulty = LANDER_DIFFICULTY_MEDIUM;
    menu_selection = 0;
    difficulty_selection = 1;
    exit_requested = 0;
    needs_redraw = 1;
    camera_x = 0;
    
    stats.landings = 0;
    stats.crashes = 0;
    stats.perfect_landings = 0;
    stats.total_fuel_saved = 0;
    stats.high_score = 0;
    
    lander_load_stats();
    lander_init_particles();
}

void lander_reset_game(void) {
    lander_generate_terrain();

    ship.x = 10.0f;
    ship.y = 2.0f;
    ship.vx = 0.2f; 
    ship.vy = 0.0f;
    ship.angle = 0.0f;
    ship.alive = 1;

    switch (difficulty) {
        case LANDER_DIFFICULTY_EASY:
            ship.fuel = MAX_FUEL * 1.5f;
            break;
        case LANDER_DIFFICULTY_MEDIUM:
            ship.fuel = MAX_FUEL;
            break;
        case LANDER_DIFFICULTY_HARD:
            ship.fuel = MAX_FUEL * 0.7f;
            break;
        case LANDER_DIFFICULTY_EXTREME:
            ship.fuel = MAX_FUEL * 0.5f;
            ship.vx = 0.8f; 
            break;
    }
    
    camera_x = 0;
    score = 0;
    bonus_multiplier = 1;
    landing_result = LANDING_NONE;
    needs_redraw = 1;
    game_start_time = pit_get_seconds();
    
    lander_init_particles();
}


void lander_generate_terrain(void) {
    terrain.height[0] = 6;
    terrain.height[TERRAIN_WIDTH - 1] = 6;
    
    int roughness = (difficulty == LANDER_DIFFICULTY_EASY) ? 3 : 
                   (difficulty == LANDER_DIFFICULTY_MEDIUM) ? 5 : 
                   (difficulty == LANDER_DIFFICULTY_HARD) ? 7 : 10;
    
    lander_generate_terrain_recursive(0, terrain.height[0], 
                                      TERRAIN_WIDTH - 1, terrain.height[TERRAIN_WIDTH - 1], 
                                      roughness);
    
    lander_place_landing_pad();
}

int lander_generate_terrain_recursive(int x1, int y1, int x2, int y2, int roughness) {
    if (x2 - x1 <= 1) return 0;
    
    int mid_x = (x1 + x2) / 2;
    int mid_y = (y1 + y2) / 2;
    
    int displacement = (lander_rand() % (roughness * 2 + 1)) - roughness;
    mid_y += displacement;

    if (mid_y < 2) mid_y = 2;
    if (mid_y > TERRAIN_HEIGHT - 5) mid_y = TERRAIN_HEIGHT - 5;
    
    terrain.height[mid_x] = mid_y;
    
    lander_generate_terrain_recursive(x1, y1, mid_x, mid_y, roughness / 2);
    lander_generate_terrain_recursive(mid_x, mid_y, x2, y2, roughness / 2);
    
    return 0;
}

void lander_place_landing_pad(void) {
    int best_x = -1;
    int min_variance = 9999;
    
    for (int x = 10; x < TERRAIN_WIDTH - 20; x++) {
        int variance = 0;
        for (int i = 0; i < LANDING_PAD_WIDTH; i++) {
            int diff = terrain.height[x + i] - terrain.height[x];
            variance += diff * diff;
        }
        
        if (variance < min_variance) {
            min_variance = variance;
            best_x = x;
        }
    }
    
    terrain.landing_pad_x = best_x;
    terrain.landing_pad_width = LANDING_PAD_WIDTH;
    
    int avg_height = 0;
    for (int i = 0; i < LANDING_PAD_WIDTH; i++) {
        avg_height += terrain.height[best_x + i];
    }
    avg_height /= LANDING_PAD_WIDTH;
    
    for (int i = 0; i < LANDING_PAD_WIDTH; i++) {
        terrain.height[best_x + i] = avg_height;
    }
}


void lander_init_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].lifetime = 0;
    }
}

void lander_spawn_particle(float x, float y, float vx, float vy, uint8_t color) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime <= 0) {
            particles[i].x = (int)x;
            particles[i].y = (int)y;
            particles[i].vx = vx;
            particles[i].vy = vy;
            particles[i].lifetime = 20 + (lander_rand() % 20);
            particles[i].color = color;
            break;
        }
    }
}

void lander_update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            particles[i].x += (int)particles[i].vx;
            particles[i].y += (int)particles[i].vy;
            particles[i].vy += 0.1f;
            particles[i].lifetime--;
        }
    }
}

void lander_spawn_exhaust(void) {
    float exhaust_x = ship.x + 2;
    float exhaust_y = ship.y + SHIP_HEIGHT;
    
    for (int i = 0; i < 3; i++) {
        float vx = (lander_randf() - 0.5f) * 0.5f;
        float vy = 0.5f + lander_randf() * 0.3f;
        uint8_t color = (lander_rand() % 2) ? VGA_YELLOW : VGA_LRED;
        lander_spawn_particle(exhaust_x, exhaust_y, vx, vy, color);
    }
}

void lander_spawn_explosion(void) {
    for (int i = 0; i < 30; i++) {
        float vx = (lander_randf() - 0.5f) * 2.0f;
        float vy = (lander_randf() - 0.5f) * 2.0f;
        uint8_t color = (lander_rand() % 3 == 0) ? VGA_LRED : 
                        (lander_rand() % 2) ? VGA_YELLOW : VGA_RED;
        lander_spawn_particle(ship.x + 2, ship.y + 1, vx, vy, color);
    }
}


void lander_update_game(void) {
    if (!ship.alive) return;
    
    lander_update_ship();
    lander_update_particles();
    lander_check_collision();
    
    camera_x = ship.x - (SCREEN_WIDTH / 2);
    if (camera_x < 0) camera_x = 0;
    if (camera_x > TERRAIN_WIDTH - SCREEN_WIDTH) camera_x = TERRAIN_WIDTH - SCREEN_WIDTH;
    
    needs_redraw = 1;
}

void lander_update_ship(void) {
    ship.vy += GRAVITY;
    ship.x += ship.vx;
    ship.y += ship.vy;
    
    if (ship.x < 0) ship.x = 0;
    if (ship.x > TERRAIN_WIDTH - SHIP_WIDTH) ship.x = TERRAIN_WIDTH - SHIP_WIDTH;
}

void lander_apply_thrust(int direction) {
    if (ship.fuel <= 0) return;
    
    ship.fuel -= FUEL_CONSUMPTION;
    if (ship.fuel < 0) ship.fuel = 0;
    
    switch (direction) {
        case 0: 
            ship.vy -= THRUST_POWER;
            lander_spawn_exhaust();
            break;
        case 1: 
            ship.vx -= THRUST_POWER * 0.5f;
            ship.angle = -10.0f;
            break;
        case 2: 
            ship.vx += THRUST_POWER * 0.5f;
            ship.angle = 10.0f;
            break;
    }
    
    if (direction != 1 && direction != 2) {
        ship.angle *= 0.9f;
    }
}

void lander_check_collision(void) {
    int ship_bottom = (int)(ship.y + SHIP_HEIGHT);
    int ship_left = (int)ship.x;
    int ship_right = (int)(ship.x + SHIP_WIDTH);
    
    for (int x = ship_left; x < ship_right && x < TERRAIN_WIDTH; x++) {
        if (ship_bottom >= TERRAIN_HEIGHT - terrain.height[x]) {
            landing_result = lander_check_landing();
            
            if (landing_result == LANDING_CRASHED) {
                ship.alive = 0;
                game_state = LANDER_CRASHED;
                stats.crashes++;
                lander_spawn_explosion();
            } else {
                ship.alive = 0;
                game_state = LANDER_LANDED;
                stats.landings++;
                if (landing_result == LANDING_PERFECT) stats.perfect_landings++;
                stats.total_fuel_saved += (int)ship.fuel;
                score = lander_calculate_score(landing_result);
                if (score > stats.high_score) stats.high_score = score;
            }
            
            lander_save_stats();
            needs_redraw = 1;
            break;
        }
    }
    
    if (ship.y < 0) {
        ship.y = 0;
        ship.vy = 0;
    }
}

LandingResult lander_check_landing(void) {
    int ship_x = (int)ship.x;
    
    if (ship_x < terrain.landing_pad_x || 
        ship_x + SHIP_WIDTH > terrain.landing_pad_x + terrain.landing_pad_width) {
        return LANDING_CRASHED;
    }
    
    float angle = lander_abs(ship.angle);
    
    if (ship.vy > MAX_SAFE_VELOCITY * 1.5f || angle > MAX_SAFE_ANGLE * 1.5f) {
        return LANDING_CRASHED;
    } else if (ship.vy < MAX_SAFE_VELOCITY * 0.3f && angle < MAX_SAFE_ANGLE * 0.3f) {
        return LANDING_PERFECT;
    } else if (ship.vy < MAX_SAFE_VELOCITY && angle < MAX_SAFE_ANGLE) {
        return LANDING_GOOD;
    } else {
        return LANDING_ROUGH;
    }
}

int lander_calculate_score(LandingResult result) {
    int base_score = 0;
    switch (result) {
        case LANDING_PERFECT: base_score = 1000; bonus_multiplier = 3; break;
        case LANDING_GOOD:    base_score = 500;  bonus_multiplier = 2; break;
        case LANDING_ROUGH:   base_score = 200;  bonus_multiplier = 1; break;
        default: base_score = 0; break;
    }
    
    int fuel_bonus = (int)ship.fuel * 5 * bonus_multiplier;
    uint32_t elapsed = pit_get_seconds() - game_start_time;
    int time_bonus = (elapsed < 30) ? (30 - elapsed) * 10 * bonus_multiplier : 0;
    
    int diff_mult = (difficulty == LANDER_DIFFICULTY_EASY) ? 1 :
                   (difficulty == LANDER_DIFFICULTY_MEDIUM) ? 2 :
                   (difficulty == LANDER_DIFFICULTY_HARD) ? 3 : 4;
    
    return (base_score + fuel_bonus + time_bonus) * diff_mult;
}


void lander_draw_menu(void) {
    vga_begin_batch();
    vga_clear();
    
    const char* title[] = {
        " _    _   _ _   _    _    ____    _        _    _   _ ____  _____ ____  ",
        "| |  | | | | \\ | |  / \\  |  _ \\  | |      / \\  | \\ | |  _ \\| ____|  _ \\ ",
        "| |  | | | |  \\| | / _ \\ | |_) | | |     / _ \\ |  \\| | | | |  _| | |_) |",
        "| |__| |_| | |\\  |/ ___ \\|  _ <  | |___ / ___ \\| |\\  | |_| | |___|  _ < ",
        "|_____\\___/|_| \\_/_/   \\_\\_| \\_\\ |_____/_/   \\_\\_| \\_|____/|_____|_| \\_\\"
    };
    
    uint8_t colors[] = {VGA_LGREY, VGA_WHITE, VGA_YELLOW, VGA_LRED, VGA_RED};
    
    for (int i = 0; i < 5; i++) {
        vga_set_color(colors[i], VGA_BLCK);
        int len = 0; while (title[i][len]) len++;
        int x = (80 - len) / 2;
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 2 + i, title[i][j]);
        }
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Navigate treacherous terrain and land safely!", 8);
    
    const char* options[] = {
        "Start Mission",
        "Select Difficulty",
        "Exit"
    };
    
    for (int i = 0; i < 3; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_fill_rect(25, 11 + i * 2, 30, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? " \x10 " : "   ";
        int x = 28;
        for (int j = 0; prefix[j]; j++) vga_putchr_at(x++, 11 + i * 2, prefix[j]);
        for (int j = 0; options[i][j]; j++) vga_putchr_at(x++, 11 + i * 2, options[i][j]);
    }
    
    if (stats.high_score > 0) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        char hs_text[40];
        const char* label = "High Score: ";
        int pos = 0; while(label[pos]) { hs_text[pos] = label[pos]; pos++; }
        
        int temp = stats.high_score;
        char digits[10]; int digit_count = 0;
        if(temp == 0) hs_text[pos++] = '0';
        else {
            while (temp > 0) { digits[digit_count++] = '0' + (temp % 10); temp /= 10; }
            while (digit_count > 0) hs_text[pos++] = digits[--digit_count];
        }
        hs_text[pos] = 0;
        vga_print_centered(hs_text, 20);
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Use UP/DOWN arrows, ENTER to select, ESC to exit", 23);
    
    vga_end_batch();
}

void lander_draw_difficulty_select(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("SELECT DIFFICULTY", 5);
    
    const char* difficulties[] = {
        "Easy   - Extra fuel, smooth terrain",
        "Medium - Standard fuel, moderate terrain",
        "Hard   - Limited fuel, rough terrain",
        "Extreme - Critical fuel, chaotic terrain",
        "Back to Menu"
    };
    
    for (int i = 0; i < 5; i++) {
        int selected = (i == difficulty_selection);
        
        if (selected) {
            vga_fill_rect(15, 9 + i * 3, 50, 1, ' ', VGA_YELLOW, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? " \x10 " : "   ";
        int x = 18;
        for (int j = 0; prefix[j]; j++) vga_putchr_at(x++, 9 + i * 3, prefix[j]);
        for (int j = 0; difficulties[i][j]; j++) vga_putchr_at(x++, 9 + i * 3, difficulties[i][j]);
        
        if (i < 4 && difficulty == i) {
            vga_set_color(VGA_LGREEN, selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(x + 1, 9 + i * 3, '\xFB');
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Use UP/DOWN arrows, ENTER to select, ESC to go back", 23);
    vga_end_batch();
}

void lander_draw_terrain(void) {
    int cam_x = (int)camera_x;
    
    for (int x = 0; x < SCREEN_WIDTH && cam_x + x < TERRAIN_WIDTH; x++) {
        int height = terrain.height[cam_x + x];
        int screen_y = VIEWPORT_OFFSET_Y + (TERRAIN_HEIGHT - height);
        
        int is_pad = (cam_x + x >= terrain.landing_pad_x && 
                      cam_x + x < terrain.landing_pad_x + terrain.landing_pad_width);
        
        vga_set_color(is_pad ? VGA_LGREEN : VGA_LGREY, VGA_BLCK);
        
        for (int y = screen_y; y < VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT; y++) {
            vga_putchr_at(x, y, is_pad ? 0xDB : 0xB2);
        }
    }
    
    int pad_screen_x = terrain.landing_pad_x - cam_x;
    if (pad_screen_x >= 0 && pad_screen_x < SCREEN_WIDTH) {
        int pad_y = VIEWPORT_OFFSET_Y + (TERRAIN_HEIGHT - terrain.height[terrain.landing_pad_x]);
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_putchr_at(pad_screen_x, pad_y - 1, '\x18');  
        vga_putchr_at(pad_screen_x + terrain.landing_pad_width - 1, pad_y - 1, '\x18');
    }
}

void lander_draw_ship(void) {
    int screen_x = (int)(ship.x - camera_x);
    int screen_y = VIEWPORT_OFFSET_Y + (int)ship.y;
    
    if (screen_x < 0 || screen_x >= SCREEN_WIDTH - SHIP_WIDTH) return;
    if (screen_y < VIEWPORT_OFFSET_Y || screen_y >= VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT) return;
    
    vga_set_color(ship.fuel > 20 ? VGA_LCYAN : VGA_LRED, VGA_BLCK);

    if (lander_abs(ship.angle) < 5) {
        vga_putchr_at(screen_x + 2, screen_y, '/'); vga_putchr_at(screen_x + 3, screen_y, '\\');
        vga_putchr_at(screen_x + 1, screen_y + 1, '|'); vga_putchr_at(screen_x + 2, screen_y + 1, 'O'); vga_putchr_at(screen_x + 3, screen_y + 1, '|');
        vga_putchr_at(screen_x + 1, screen_y + 2, '/'); vga_putchr_at(screen_x + 3, screen_y + 2, '\\');
    } else if (ship.angle < 0) {
        vga_putchr_at(screen_x + 1, screen_y, '/'); vga_putchr_at(screen_x + 2, screen_y, '\\');
        vga_putchr_at(screen_x, screen_y + 1, '|'); vga_putchr_at(screen_x + 1, screen_y + 1, 'O'); vga_putchr_at(screen_x + 2, screen_y + 1, '|');
        vga_putchr_at(screen_x, screen_y + 2, '/'); vga_putchr_at(screen_x + 2, screen_y + 2, '|'); vga_putchr_at(screen_x + 3, screen_y + 2, '\\');
    } else { 
        vga_putchr_at(screen_x + 2, screen_y, '/'); vga_putchr_at(screen_x + 3, screen_y, '\\');
        vga_putchr_at(screen_x + 1, screen_y + 1, '|'); vga_putchr_at(screen_x + 2, screen_y + 1, 'O'); vga_putchr_at(screen_x + 3, screen_y + 1, '|');
        vga_putchr_at(screen_x + 1, screen_y + 2, '/'); vga_putchr_at(screen_x + 2, screen_y + 2, '|'); vga_putchr_at(screen_x + 4, screen_y + 2, '\\');
    }
}

void lander_draw_particles(void) {
    int cam_x = (int)camera_x;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            int sx = particles[i].x - cam_x;
            int sy = VIEWPORT_OFFSET_Y + particles[i].y;
            if (sx >= 0 && sx < SCREEN_WIDTH && sy >= VIEWPORT_OFFSET_Y && sy < VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT) {
                vga_set_color(particles[i].color, VGA_BLCK);
                vga_putchr_at(sx, sy, '*');
            }
        }
    }
}

void lander_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== LUNAR LANDER ===", 0);
    
    const char* diff_names[] = {"EASY", "MEDIUM", "HARD", "EXTREME"};
    vga_set_color(VGA_LCYAN, VGA_BLUE);
    vga_print_centered(diff_names[difficulty], 1);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_at("FUEL:", 2, VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT + 1);
    
    vga_draw_progress_bar(8, VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT + 1, 20, 
                          (int)((ship.fuel / MAX_FUEL) * 100), 
                          ship.fuel > 30 ? VGA_LGREEN : VGA_LRED, VGA_DGREY, VGA_BLCK);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_at("Vel:", 32, VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT + 1);
    vga_print_int_at((int)(ship.vy * 10), 37, VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT + 1);
    
    vga_print_at("Score:", 60, VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT + 1);
    vga_print_int_at(score, 67, VIEWPORT_OFFSET_Y + TERRAIN_HEIGHT + 1);
}

void lander_draw_minimap(void) {
    int mm_x = 20;
    int mm_y = 1;
    int mm_width = 40;
    int mm_height = 4;

    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_draw_box_single(mm_x - 1, mm_y - 1, mm_width + 2, mm_height + 2, VGA_DGREY, VGA_BLCK);

    for (int i = 0; i < mm_width; i++) {
        int h = terrain.height[i * 4] / 5;
        if (h > mm_height) h = mm_height;
        vga_putchr_at(mm_x + i, mm_y + mm_height - h, '.');
    }

    int pad_x = terrain.landing_pad_x / 4;
    int pad_w = terrain.landing_pad_width / 4; if(pad_w < 1) pad_w = 1;
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    for(int i=0; i<pad_w; i++) 
        vga_putchr_at(mm_x + pad_x + i, mm_y + mm_height - (terrain.height[terrain.landing_pad_x]/5), '_');

    int sx = (int)ship.x / 4;
    int sy = mm_y + (int)((ship.y / TERRAIN_HEIGHT) * mm_height);
    if(sy > mm_y + mm_height) sy = mm_y + mm_height;
    if(sy < mm_y) sy = mm_y;
    
    if(sx >= 0 && sx < mm_width) {
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_putchr_at(mm_x + sx, sy, '+');
    }
}

void lander_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    lander_draw_terrain();
    if(ship.alive) lander_draw_ship();
    lander_draw_particles();
    lander_draw_ui();
    lander_draw_minimap();

    if (game_state == LANDER_PAUSED) {
        lander_draw_paused();
    } else if (game_state == LANDER_LANDED || game_state == LANDER_CRASHED) {
        lander_draw_landing_result();
    }

    vga_end_batch();
}

void lander_draw_landing_result(void) {
    int w = 40; int h = 10;
    int x = (SCREEN_WIDTH - w) / 2;
    int y = (SCREEN_HEIGHT - h) / 2;

    uint8_t c = (landing_result == LANDING_CRASHED) ? VGA_RED : VGA_GREEN;
    vga_draw_box_double(x, y, w, h, c, VGA_BLCK);
    vga_fill_rect(x+1, y+1, w-2, h-2, ' ', c, VGA_BLCK); 

    vga_set_color(c, VGA_BLCK);
    const char* msg = (landing_result == LANDING_CRASHED) ? "CRASHED!" : "SUCCESS!";
    vga_print_centered(msg, y + 2);

    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_centered("Press ENTER to Retry", y + 5);
    vga_print_centered("ESC for Menu", y + 6);
}

void lander_draw_paused(void) {
    vga_draw_box_single(30, 10, 20, 3, VGA_WHITE, VGA_BLUE);
    vga_set_color(VGA_WHITE, VGA_BLUE);
    vga_print_centered("PAUSED", 11);
}


void lander_handle_input(void) {
    keyboard_poll();

    while (has_key_event()) {
        KeyEvent k = get_key_event();
        char c = get_char();
        int update = 0;

        if (game_state == LANDER_MENU) {
            if (k.key_code == KEY_ARROW_UP) { 
                menu_selection--; 
                if(menu_selection < 0) menu_selection = 2; 
                update = 1;
            }
            else if (k.key_code == KEY_ARROW_DOWN) { 
                menu_selection++; 
                if(menu_selection > 2) menu_selection = 0; 
                update = 1;
            }
            else if (k.key_code == KEY_ENTER || k.character == '\n') {
                if (menu_selection == 0) { lander_reset_game(); game_state = LANDER_PLAYING; }
                else if (menu_selection == 1) { game_state = LANDER_DIFFICULTY_SELECT; }
                else if (menu_selection == 2) { exit_requested = 1; }
                update = 1;
            }
            else if (c == 27) {
                exit_requested = 1;
            }
        } 
        else if (game_state == LANDER_DIFFICULTY_SELECT) {
            if (k.key_code == KEY_ARROW_UP) { 
                difficulty_selection--; 
                if(difficulty_selection < 0) difficulty_selection = 4; 
                update = 1;
            }
            else if (k.key_code == KEY_ARROW_DOWN) { 
                difficulty_selection++; 
                if(difficulty_selection > 4) difficulty_selection = 0; 
                update = 1;
            }
            else if (k.key_code == KEY_ENTER || k.character == '\n') {
                if(difficulty_selection < 4) difficulty = (LanderDifficulty)difficulty_selection;
                game_state = LANDER_MENU; 
                update = 1;
            }
            else if (c == 27) {
                game_state = LANDER_MENU; 
                update = 1;
            }
        }
        else if (game_state == LANDER_LANDED || game_state == LANDER_CRASHED) {
            if (k.key_code == KEY_ENTER || k.character == '\n') { lander_reset_game(); game_state = LANDER_PLAYING; update = 1; }
            if (c == 27) { game_state = LANDER_MENU; update = 1; }
        }
        else if (game_state == LANDER_PLAYING) {
            if (c == 27) { game_state = LANDER_MENU; update = 1; }
            if (k.character == 'p') { game_state = LANDER_PAUSED; update = 1; }
        }
        else if (game_state == LANDER_PAUSED) {
            if (k.character == 'p') { game_state = LANDER_PLAYING; update = 1; }
            if (c == 27) { game_state = LANDER_MENU; update = 1; }
        }

        if(update) needs_redraw = 1;
    }

    if (game_state == LANDER_PLAYING && ship.alive) {
        if (is_up_pressed()) lander_apply_thrust(0); 
        if (is_left_pressed()) lander_apply_thrust(1);
        if (is_right_pressed()) lander_apply_thrust(2);
    }
}

void lander_save_stats(void) {}
void lander_load_stats(void) {}

void lunar_lander_cleanup(void) {
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_clear();
}

void lunar_lander_run(void) {
    clear_event_buffer();
    
    lunar_lander_init();
    
    while (!exit_requested) {
        lander_handle_input();
        
        if (game_state == LANDER_PLAYING) {
            lander_update_game();
        }

        if (needs_redraw || game_state == LANDER_PLAYING) {
            switch (game_state) {
                case LANDER_MENU: 
                    lander_draw_menu(); 
                    break;
                case LANDER_DIFFICULTY_SELECT: 
                    lander_draw_difficulty_select(); 
                    break;
                case LANDER_PLAYING: 
                case LANDER_PAUSED: 
                case LANDER_LANDED: 
                case LANDER_CRASHED: 
                    lander_draw_game(); 
                    break;
                default: 
                    break;
            }
            needs_redraw = 0;
        }

        pit_delay_ms(33); 
    }
    
    lunar_lander_cleanup();
}