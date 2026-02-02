#include "sos_spaceshooter.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_fat16.h"

#define GAME_SPACES_SAVE_FILE "SPACESSAVE.DAT"

static Player player;
static Boss boss;
static Bullet bullets[MAX_BULLETS];
static Enemy enemies[MAX_ENEMIES];
static SS_Powerup powerups[MAX_POWERUPS];
static Explosion explosions[MAX_EXPLOSIONS];
static Particle particles[MAX_PARTICLES];
static Star stars[MAX_STARS];
static WaveSystem wave_system;

static ShooterState game_state;
static int menu_selection;
static int high_score;
static int level;
static uint32_t last_update_time;
static int difficulty_multiplier;

static uint32_t shooter_rand_seed = 98765;

uint32_t spaceshooter_rand(void) {
    shooter_rand_seed = (shooter_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return shooter_rand_seed;
}

float spaceshooter_randf(void) {
    return (float)(spaceshooter_rand() % 1000) / 1000.0f;
}

float spaceshooter_abs(float x) {
    return x < 0 ? -x : x;
}


static inline float wrap_pi(float x) {
    while (x > SS_PI)  x -= SS_TWO_PI;
    while (x < -SS_PI) x += SS_TWO_PI;
    return x;
}

float sinf(float x) {
    x = wrap_pi(x);

    float x2 = x * x;
    float x3 = x * x2;
    float x5 = x3 * x2;
    float x7 = x5 * x2;

    return x
         - x3 * (1.0f / 6.0f)
         + x5 * (1.0f / 120.0f)
         - x7 * (1.0f / 5040.0f);
}

static inline float cosf_internal(float x) {
    x = wrap_pi(x);

    float x2 = x * x;
    float x4 = x2 * x2;
    float x6 = x4 * x2;

    return 1.0f
         - x2 * 0.5f
         + x4 * (1.0f / 24.0f)
         - x6 * (1.0f / 720.0f);
}

void sincosf(float x, float *s, float *c) {
    x = wrap_pi(x);

    float x2 = x * x;

    /* sine */
    float x3 = x * x2;
    float x5 = x3 * x2;
    float x7 = x5 * x2;

    *s = x
       - x3 * (1.0f / 6.0f)
       + x5 * (1.0f / 120.0f)
       - x7 * (1.0f / 5040.0f);

    /* cosine */
    float x4 = x2 * x2;
    float x6 = x4 * x2;

    *c = 1.0f
       - x2 * 0.5f
       + x4 * (1.0f / 24.0f)
       - x6 * (1.0f / 720.0f);
}




void spaceshooter_game_init(void) {
    game_state = SHOOTER_MENU;
    menu_selection = 0;
    high_score = 0;
    spaceshooter_load_high_score();

    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = spaceshooter_randf() * SS_GAME_WIDTH;
        stars[i].y = spaceshooter_randf() * SS_GAME_HEIGHT;
        stars[i].vy = 0.1f + spaceshooter_randf() * 0.3f;
        
        int brightness = spaceshooter_rand() % 3;
        if (brightness == 0) {
            stars[i].color = VGA_DGREY;
            stars[i].symbol = '.';
        } else if (brightness == 1) {
            stars[i].color = VGA_LGREY;
            stars[i].symbol = '.';
        } else {
            stars[i].color = VGA_WHITE;
            stars[i].symbol = '*';
        }
    }
}

void spaceshooter_reset_game(void) {

    player.x = PLAYER_START_X;
    player.y = PLAYER_START_Y;
    player.health = PLAYER_MAX_HEALTH;
    player.max_health = PLAYER_MAX_HEALTH;
    player.score = 0;
    player.lives = 3;
    player.color = VGA_LCYAN;
    player.shoot_cooldown = 0;
    player.invulnerable_timer = 0;
    player.weapon_level = 1;
    player.weapon_type = 0; 
    player.shield_active = 0;
    player.rapid_fire = 0;

    boss.active = 0;

    for (int i = 0; i < MAX_BULLETS; i++) {
        bullets[i].active = 0;
    }
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].active = 0;
    }
    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerups[i].active = 0;
    }
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        explosions[i].active = 0;
    }
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = 0;
    }

    wave_system.wave_number = 1;
    wave_system.enemies_spawned = 0;
    wave_system.enemies_killed = 0;
    wave_system.enemies_to_spawn = 5;
    wave_system.spawn_timer = 0;
    wave_system.boss_wave = 0;
    
    level = 1;
    difficulty_multiplier = 1;
    last_update_time = pit_get_total_milliseconds();
}


void spaceshooter_update_game(void) {
    uint32_t current_time = pit_get_total_milliseconds();

    if (player.shield_active && current_time >= player.shield_end_time) {
        player.shield_active = 0;
    }
    if (player.rapid_fire && current_time >= player.rapid_fire_end_time) {
        player.rapid_fire = 0;
    }

    if (player.invulnerable_timer > 0) {
        player.invulnerable_timer--;
    }

    if (player.shoot_cooldown > 0) {
        player.shoot_cooldown--;
    }
    
    spaceshooter_update_wave_system();
    spaceshooter_update_stars();
    spaceshooter_update_player();
    spaceshooter_update_bullets();
    spaceshooter_update_enemies();
    spaceshooter_update_powerups();
    spaceshooter_update_explosions();
    spaceshooter_update_particles();
    spaceshooter_update_boss();
    
    spaceshooter_check_bullet_collisions();
    spaceshooter_check_enemy_collisions();
    spaceshooter_check_powerup_collisions();
    spaceshooter_check_boss_collisions();
}

void spaceshooter_update_wave_system(void) {
    if (boss.active) return;  

    if (wave_system.enemies_spawned >= wave_system.enemies_to_spawn) {
        int all_dead = 1;
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].active) {
                all_dead = 0;
                break;
            }
        }
        
        if (all_dead) {

            wave_system.wave_number++;
            wave_system.enemies_spawned = 0;
            wave_system.enemies_killed = 0;
            wave_system.enemies_to_spawn = 5 + (wave_system.wave_number * 2);
            wave_system.spawn_timer = 0;

            if (wave_system.wave_number % 5 == 0) {
                wave_system.boss_wave = 1;
                spaceshooter_spawn_boss();
                game_state = SHOOTER_BOSS_FIGHT;
            }
            
            difficulty_multiplier = 1 + (wave_system.wave_number / 3);
        }
    } else {

        wave_system.spawn_timer++;
        if (wave_system.spawn_timer >= ENEMY_SPAWN_RATE - (wave_system.wave_number * 3)) {
            wave_system.spawn_timer = 0;
            
            int type = spaceshooter_rand() % 7;

            if (wave_system.wave_number > 5) {
                type = 2 + (spaceshooter_rand() % 5);
            }
            
            spaceshooter_spawn_enemy(type);
            wave_system.enemies_spawned++;
        }
    }
}

void spaceshooter_update_player(void) {

    if (player.x < 0) player.x = 0;
    if (player.x >= SS_GAME_WIDTH) player.x = SS_GAME_WIDTH - 1;
    if (player.y < 0) player.y = 0;
    if (player.y >= SS_GAME_HEIGHT) player.y = SS_GAME_HEIGHT - 1;
}

void spaceshooter_update_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        
        bullets[i].x += bullets[i].vx;
        bullets[i].y += bullets[i].vy;

        if (bullets[i].y < 0 || bullets[i].y >= SS_GAME_HEIGHT ||
            bullets[i].x < 0 || bullets[i].x >= SS_GAME_WIDTH) {
            bullets[i].active = 0;
        }
    }
}

void spaceshooter_update_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) continue;

        switch (enemies[i].type) {
            case ENEMY_BASIC:
                enemies[i].y += enemies[i].vy;
                break;
                
            case ENEMY_FAST:
                enemies[i].y += enemies[i].vy * 1.5f;
                break;
                
            case ENEMY_TANK:
                enemies[i].y += enemies[i].vy * 0.7f;
                break;
                
            case ENEMY_ZIGZAG:
                enemies[i].y += enemies[i].vy;
                enemies[i].angle += 0.1f;
                enemies[i].x += 2.0f * __builtin_sinf(enemies[i].angle);
                break;
                
            case ENEMY_SPINNER:
                enemies[i].y += enemies[i].vy * 0.8f;
                enemies[i].angle += 0.2f;
                break;
                
            case ENEMY_KAMIKAZE:
                if (enemies[i].y < player.y - 5) {
                    float dx = player.x - enemies[i].x;
                    float dy = player.y - enemies[i].y;
                    float dist = __builtin_sqrtf(dx*dx + dy*dy);
                    if (dist > 0.1f) {
                        enemies[i].vx = (dx / dist) * 0.4f;
                        enemies[i].vy = (dy / dist) * 0.4f;
                    }
                }
                enemies[i].x += enemies[i].vx;
                enemies[i].y += enemies[i].vy;
                break;
                
            case ENEMY_SHOOTER:
                enemies[i].y += enemies[i].vy * 0.5f;
                enemies[i].shoot_timer++;
                if (enemies[i].shoot_timer >= 60) {
                    enemies[i].shoot_timer = 0;
                    spaceshooter_spawn_bullet(enemies[i].x, enemies[i].y + 1, 
                                            0, 0.5f, 0, VGA_RED, 10);
                }
                break;
        }

        if (enemies[i].y >= SS_GAME_HEIGHT || enemies[i].x < -5 || enemies[i].x >= SS_GAME_WIDTH + 5) {
            enemies[i].active = 0;
        }
    }
}

void spaceshooter_update_powerups(void) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) continue;
        
        powerups[i].y += powerups[i].vy;

        if (powerups[i].y >= SS_GAME_HEIGHT) {
            powerups[i].active = 0;
        }
    }
}

void spaceshooter_update_explosions(void) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;
        
        explosions[i].frame++;
        if (explosions[i].frame >= explosions[i].max_frames) {
            explosions[i].active = 0;
        }
    }
}

void spaceshooter_update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].lifetime--;
        
        if (particles[i].lifetime <= 0 || 
            particles[i].y < 0 || particles[i].y >= SS_GAME_HEIGHT) {
            particles[i].active = 0;
        }
    }
}

void spaceshooter_update_stars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].y += stars[i].vy;
        
        if (stars[i].y >= SS_GAME_HEIGHT) {
            stars[i].y = 0;
            stars[i].x = spaceshooter_randf() * SS_GAME_WIDTH;
        }
    }
}

void spaceshooter_update_boss(void) {
    if (!boss.active) return;
    
    boss.move_timer++;
    boss.attack_timer++;

    if (boss.move_timer >= 30) {
        boss.move_timer = 0;
        boss.target_x = spaceshooter_randf() * (SS_GAME_WIDTH - 10) + 5;
    }

    if (boss.x < boss.target_x - 0.5f) {
        boss.x += 0.3f;
    } else if (boss.x > boss.target_x + 0.5f) {
        boss.x -= 0.3f;
    }
 
    if (boss.attack_timer >= 40) {
        boss.attack_timer = 0;
        
        if (boss.phase == 0) {

            for (int i = -2; i <= 2; i++) {
                spaceshooter_spawn_bullet(boss.x, boss.y + 3, 
                                        i * 0.15f, 0.6f, 0, VGA_LMAGENTA, 15);
            }
        } else if (boss.phase == 1) {

            for (int i = 0; i < 8; i++) {
                float angle = (i / 8.0f) * 6.28f;
                spaceshooter_spawn_bullet(boss.x, boss.y + 3,
                                        __builtin_cosf(angle) * 0.4f,
                                        __builtin_sinf(angle) * 0.4f + 0.2f,
                                        0, VGA_LRED, 15);
            }
        } else {

            spaceshooter_spawn_bullet(boss.x - 3, boss.y + 3, -0.2f, 0.7f, 0, VGA_RED, 15);
            spaceshooter_spawn_bullet(boss.x, boss.y + 3, 0, 0.7f, 0, VGA_RED, 15);
            spaceshooter_spawn_bullet(boss.x + 3, boss.y + 3, 0.2f, 0.7f, 0, VGA_RED, 15);
        }
    }

    if (boss.health < boss.max_health * 0.66f && boss.phase == 0) {
        boss.phase = 1;
        spaceshooter_spawn_particles(boss.x, boss.y, 20, VGA_YELLOW);
    } else if (boss.health < boss.max_health * 0.33f && boss.phase == 1) {
        boss.phase = 2;
        spaceshooter_spawn_particles(boss.x, boss.y, 30, VGA_RED);
    }

    if (boss.health <= 0) {
        boss.active = 0;
        player.score += 5000;
        spaceshooter_spawn_explosion(boss.x, boss.y, VGA_LRED);
        spaceshooter_spawn_particles(boss.x, boss.y, 50, VGA_YELLOW);

        for (int i = 0; i < 5; i++) {
            spaceshooter_spawn_powerup(boss.x + (spaceshooter_randf() - 0.5f) * 10,
                                      boss.y, spaceshooter_rand() % 7);
        }
        
        game_state = SHOOTER_LEVEL_COMPLETE;
        wave_system.boss_wave = 0;
    }
}


void spaceshooter_move_player(int dx, int dy) {
    player.x += dx * PLAYER_SPEED;
    player.y += dy * PLAYER_SPEED;
}

void spaceshooter_player_shoot(void) {
    if (player.shoot_cooldown > 0) return;
    
    int cooldown = player.rapid_fire ? 3 : 8;
    player.shoot_cooldown = cooldown;
    
    if (player.weapon_type == 0) {
        if (player.weapon_level == 1) {
            spaceshooter_spawn_bullet(player.x, player.y - 1, 0, -BULLET_SPEED, 1, VGA_YELLOW, 10);
        } else if (player.weapon_level == 2) {
            spaceshooter_spawn_bullet(player.x - 1, player.y - 1, 0, -BULLET_SPEED, 1, VGA_YELLOW, 15);
            spaceshooter_spawn_bullet(player.x + 1, player.y - 1, 0, -BULLET_SPEED, 1, VGA_YELLOW, 15);
        } else {
            spaceshooter_spawn_bullet(player.x - 1, player.y - 1, 0, -BULLET_SPEED, 1, VGA_YELLOW, 20);
            spaceshooter_spawn_bullet(player.x, player.y - 1, 0, -BULLET_SPEED * 1.2f, 1, VGA_LCYAN, 20);
            spaceshooter_spawn_bullet(player.x + 1, player.y - 1, 0, -BULLET_SPEED, 1, VGA_YELLOW, 20);
        }
    } else if (player.weapon_type == 1) {
        for (int i = -2; i <= 2; i++) {
            spaceshooter_spawn_bullet(player.x, player.y - 1, 
                                    i * 0.3f, -BULLET_SPEED, 1, VGA_LGREEN, 12);
        }
    } else if (player.weapon_type == 2) {
        spaceshooter_spawn_bullet(player.x, player.y - 1, 0, -BULLET_SPEED * 2, 1, VGA_LCYAN, 25);
    }
}

void spaceshooter_lose_life(void) {
    if (player.invulnerable_timer > 0) return;
    
    player.lives--;
    player.invulnerable_timer = 60;  
    player.health = player.max_health;
    
    spaceshooter_spawn_explosion(player.x, player.y, VGA_YELLOW);
    spaceshooter_spawn_particles(player.x, player.y, 15, VGA_LCYAN);

    player.shield_active = 0;
    player.weapon_type = 0;
    
    if (player.lives <= 0) {
        game_state = SHOOTER_GAME_OVER;
        if (player.score > high_score) {
            high_score = player.score;
            spaceshooter_save_high_score();
        }
    }
}


void spaceshooter_spawn_enemy(int type) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].active) {
            enemies[i].active = 1;
            enemies[i].x = spaceshooter_randf() * (SS_GAME_WIDTH - 4) + 2;
            enemies[i].y = -2;
            enemies[i].vx = 0;
            enemies[i].vy = 0.2f + spaceshooter_randf() * 0.1f;
            enemies[i].type = type;
            enemies[i].health = spaceshooter_get_enemy_health(type);
            enemies[i].max_health = enemies[i].health;
            enemies[i].color = spaceshooter_get_enemy_color(type);
            enemies[i].symbol = spaceshooter_get_enemy_symbol(type);
            enemies[i].shoot_timer = 0;
            enemies[i].points = spaceshooter_get_enemy_points(type);
            enemies[i].angle = 0;
            break;
        }
    }
}

void spaceshooter_spawn_powerup(float x, float y, int type) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) {
            powerups[i].active = 1;
            powerups[i].x = x;
            powerups[i].y = y;
            powerups[i].vy = 0.2f;
            powerups[i].type = type;
            
            switch (type) {
                case SS_POWERUP_HEALTH:
                    powerups[i].color = VGA_LRED;
                    powerups[i].symbol = '+';
                    break;
                case SS_POWERUP_WEAPON_UP:
                    powerups[i].color = VGA_YELLOW;
                    powerups[i].symbol = 'W';
                    break;
                case SS_POWERUP_SHIELD:
                    powerups[i].color = VGA_LBLUE;
                    powerups[i].symbol = 'S';
                    break;
                case SS_POWERUP_RAPID_FIRE:
                    powerups[i].color = VGA_LGREEN;
                    powerups[i].symbol = 'R';
                    break;
                case SS_POWERUP_SPREAD_SHOT:
                    powerups[i].color = VGA_LMAGENTA;
                    powerups[i].symbol = 'P';
                    break;
                case SS_POWERUP_LASER:
                    powerups[i].color = VGA_LCYAN;
                    powerups[i].symbol = 'L';
                    break;
                case SS_POWERUP_LIFE:
                    powerups[i].color = VGA_LGREEN;
                    powerups[i].symbol = 'E';
                    break;
            }
            break;
        }
    }
}

void spaceshooter_spawn_explosion(float x, float y, uint8_t color) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) {
            explosions[i].active = 1;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].frame = 0;
            explosions[i].max_frames = 12;
            explosions[i].color = color;
            break;
        }
    }
}

void spaceshooter_spawn_particles(float x, float y, int count, uint8_t color) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!particles[i].active) {
            particles[i].active = 1;
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = (spaceshooter_randf() - 0.5f) * 0.8f;
            particles[i].vy = (spaceshooter_randf() - 0.5f) * 0.8f;
            particles[i].lifetime = 15 + spaceshooter_rand() % 20;
            particles[i].color = color;
            particles[i].symbol = (spaceshooter_rand() % 2) ? '.' : '*';
            spawned++;
        }
    }
}

void spaceshooter_spawn_bullet(float x, float y, float vx, float vy, 
                               int is_player, uint8_t color, int damage) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].active = 1;
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].vx = vx;
            bullets[i].vy = vy;
            bullets[i].color = color;
            bullets[i].is_player_bullet = is_player;
            bullets[i].damage = damage;
            break;
        }
    }
}

void spaceshooter_spawn_boss(void) {
    boss.active = 1;
    boss.x = SS_GAME_WIDTH / 2;
    boss.y = 3;
    boss.health = 500 + (wave_system.wave_number * 100);
    boss.max_health = boss.health;
    boss.phase = 0;
    boss.attack_timer = 0;
    boss.move_timer = 0;
    boss.target_x = boss.x;
    boss.color = VGA_LMAGENTA;
}


void spaceshooter_check_bullet_collisions(void) {
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active) continue;
        
        if (bullets[b].is_player_bullet) {
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!enemies[e].active) continue;
                
                int dx = spaceshooter_abs(bullets[b].x - enemies[e].x);
                int dy = spaceshooter_abs(bullets[b].y - enemies[e].y);
                
                if (dx < 1 && dy < 1) {
                    enemies[e].health -= bullets[b].damage;
                    bullets[b].active = 0;
                    
                    if (enemies[e].health <= 0) {
                        enemies[e].active = 0;
                        player.score += enemies[e].points;
                        wave_system.enemies_killed++;
                        spaceshooter_spawn_explosion(enemies[e].x, enemies[e].y, enemies[e].color);
                        spaceshooter_spawn_particles(enemies[e].x, enemies[e].y, 8, enemies[e].color);

                        if (spaceshooter_rand() % 8 == 0) {
                            spaceshooter_spawn_powerup(enemies[e].x, enemies[e].y, 
                                                      spaceshooter_rand() % 7);
                        }
                    } else {
                        spaceshooter_spawn_particles(enemies[e].x, enemies[e].y, 3, VGA_WHITE);
                    }
                    break;
                }
            }
        } else {
            int dx = spaceshooter_abs(bullets[b].x - player.x);
            int dy = spaceshooter_abs(bullets[b].y - player.y);
            
            if (dx < 1 && dy < 1 && player.invulnerable_timer == 0) {
                bullets[b].active = 0;
                
                if (!player.shield_active) {
                    player.health -= bullets[b].damage;
                    if (player.health <= 0) {
                        spaceshooter_lose_life();
                    }
                    spaceshooter_spawn_particles(player.x, player.y, 5, VGA_RED);
                }
            }
        }
    }
}

void spaceshooter_check_enemy_collisions(void) {
    for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!enemies[e].active) continue;
        
        int dx = spaceshooter_abs(enemies[e].x - player.x);
        int dy = spaceshooter_abs(enemies[e].y - player.y);
        
        if (dx < 1 && dy < 1 && player.invulnerable_timer == 0) {
            enemies[e].active = 0;
            spaceshooter_spawn_explosion(enemies[e].x, enemies[e].y, enemies[e].color);
            
            if (!player.shield_active) {
                player.health -= 25;
                if (player.health <= 0) {
                    spaceshooter_lose_life();
                }
            }
        }
    }
}

void spaceshooter_check_powerup_collisions(void) {
    for (int p = 0; p < MAX_POWERUPS; p++) {
        if (!powerups[p].active) continue;
        
        int dx = spaceshooter_abs(powerups[p].x - player.x);
        int dy = spaceshooter_abs(powerups[p].y - player.y);
        
        if (dx < 2 && dy < 1) {
            powerups[p].active = 0;
            uint32_t current_time = pit_get_total_milliseconds();
            
            switch (powerups[p].type) {
                case SS_POWERUP_HEALTH:
                    player.health += 30;
                    if (player.health > player.max_health) player.health = player.max_health;
                    break;
                case SS_POWERUP_WEAPON_UP:
                    player.weapon_level++;
                    if (player.weapon_level > 3) player.weapon_level = 3;
                    break;
                case SS_POWERUP_SHIELD:
                    player.shield_active = 1;
                    player.shield_end_time = current_time + POWERUP_DURATION;
                    break;
                case SS_POWERUP_RAPID_FIRE:
                    player.rapid_fire = 1;
                    player.rapid_fire_end_time = current_time + POWERUP_DURATION;
                    break;
                case SS_POWERUP_SPREAD_SHOT:
                    player.weapon_type = 1;
                    break;
                case SS_POWERUP_LASER:
                    player.weapon_type = 2;
                    break;
                case SS_POWERUP_LIFE:
                    player.lives++;
                    break;
            }
            spaceshooter_spawn_particles(powerups[p].x, powerups[p].y, 10, powerups[p].color);
        }
    }
}

void spaceshooter_check_boss_collisions(void) {
    if (!boss.active) return;
    for (int b = 0; b < MAX_BULLETS; b++) {
        if (!bullets[b].active || !bullets[b].is_player_bullet) continue;
        
        if (bullets[b].x >= boss.x - 5 && bullets[b].x <= boss.x + 5 &&
            bullets[b].y >= boss.y && bullets[b].y <= boss.y + 3) {
            boss.health -= bullets[b].damage;
            bullets[b].active = 0;
            spaceshooter_spawn_particles(bullets[b].x, bullets[b].y, 5, VGA_YELLOW);
        }
    }
}

void spaceshooter_handle_input(void) {
    keyboard_poll();
    if (game_state == SHOOTER_MENU) {
        if (!has_key()) return;
        char c = get_char();
        
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 2;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 2) menu_selection = 0;
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                spaceshooter_reset_game();
                game_state = SHOOTER_PLAYING;
            } else if (menu_selection == 1) {
                //TODO Implement instructions
            } else if (menu_selection == 2) {
                game_state = SHOOTER_EXIT;
            }
        } else if (c == 27) {  
            game_state = SHOOTER_EXIT;
        }
    } else if (game_state == SHOOTER_PLAYING || game_state == SHOOTER_BOSS_FIGHT) {
        if (has_key()) {
            char c = get_char();
            
            if (c == 0x11 || c == 'w' || c == 'W') {  
                spaceshooter_move_player(0, -1);
            } else if (c == 0x12 || c == 's' || c == 'S') {  
                spaceshooter_move_player(0, 1);
            } else if (c == 0x13 || c == 'a' || c == 'A') { 
                spaceshooter_move_player(-1, 0);
            } else if (c == 0x14 || c == 'd' || c == 'D') {  
                spaceshooter_move_player(1, 0);
            } else if (c == ' ') { 
                spaceshooter_player_shoot();
            } else if (c == 'p' || c == 'P') {
                game_state = SHOOTER_PAUSED;
            } else if (c == 27) {  
                game_state = SHOOTER_MENU;
            }
        }
    } else if (game_state == SHOOTER_PAUSED) {
        if (!has_key()) return;
        char c = get_char();
        
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = boss.active ? SHOOTER_BOSS_FIGHT : SHOOTER_PLAYING;
            last_update_time = pit_get_total_milliseconds();
        } else if (c == 27) {
            game_state = SHOOTER_MENU;
        }
    } else if (game_state == SHOOTER_LEVEL_COMPLETE) {
        if (!has_key()) return;
        char c = get_char();
        
        if (c == '\n' || c == ' ') {
            level++;
            game_state = SHOOTER_PLAYING;
        } else if (c == 27) {
            game_state = SHOOTER_MENU;
        }
    }
}

void spaceshooter_draw_menu(void) {
    vga_clear();
    const char* title[] = {
        " ____  ____   _    ____ _____   ____  _   _  ___   ___ _____ _____ ____  ",
        "/ ___||  _ \\ / \\  / ___| ____| / ___|| | | |/ _ \\ / _ \\_   _| ____|  _ \\ ",
        "\\___ \\| |_) / _ \\| |   |  _|   \\___ \\| |_| | | | | | | || | |  _| | |_) |",
        " ___) |  __/ ___ \\ |___| |___   ___) |  _  | |_| | |_| || | | |___|  _ < ",
        "|____/|_| /_/   \\_\\____|_____| |____/|_| |_|\\___/ \\___/ |_| |_____|_| \\_\\"
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

    for (int i = 0; i < 30; i++) {
        int x = spaceshooter_rand() % 80;
        int y = 8 + (spaceshooter_rand() % 10);
        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_putchr_at(x, y, '.');
    }

    const char* options[] = {
        "Start Game",
        "Instructions",
        "Exit"
    };

    for (int i = 0; i < 3; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(28, 12 + i * 2, 24, 1, ' ', VGA_YELLOW, VGA_BLUE);
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
    char hs[40];
    int pos = 0;
    const char* hs_label = "High Score: ";
    for (int i = 0; hs_label[i]; i++) hs[pos++] = hs_label[i];

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

    int len = 0;
    while (hs[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 20, hs[i]);
    }

    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* hint = "Use UP/DOWN arrows, ENTER to select, ESC to exit";
    len = 0;
    while (hint[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, hint[i]);
    }
}

void spaceshooter_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    spaceshooter_draw_stars();
    spaceshooter_draw_field();
    spaceshooter_draw_enemies();
    spaceshooter_draw_boss();
    spaceshooter_draw_bullets();
    spaceshooter_draw_player();
    spaceshooter_draw_powerups();
    spaceshooter_draw_explosions();
    spaceshooter_draw_particles();
    spaceshooter_draw_ui();

    vga_end_batch();
}

void spaceshooter_draw_field(void) {
    vga_draw_box_double(SS_GAME_OFFSET_X, SS_GAME_OFFSET_Y,
    SS_GAME_WIDTH, SS_GAME_HEIGHT, VGA_CYAN, VGA_BLCK);
}

void spaceshooter_draw_player(void) {
    if (player.invulnerable_timer > 0 && (player.invulnerable_timer / 4) % 2 == 0) {
    return;  
    }
    int px = SS_GAME_OFFSET_X + (int)player.x;
    int py = SS_GAME_OFFSET_Y + (int)player.y;
    
    uint8_t color = player.shield_active ? VGA_LBLUE : player.color;
    
    vga_set_color(color, VGA_BLCK);
    vga_putchr_at(px, py - 1, '^');
    vga_putchr_at(px - 1, py, '<');
    vga_putchr_at(px, py, 0x01);  
    vga_putchr_at(px + 1, py, '>');
    
    if (player.shield_active) {
        vga_set_color(VGA_LBLUE, VGA_BLCK);
        vga_putchr_at(px - 1, py - 1, '/');
        vga_putchr_at(px + 1, py - 1, '\\');
        vga_putchr_at(px - 2, py, '(');
        vga_putchr_at(px + 2, py, ')');
    }
}

void spaceshooter_draw_bullets(void) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) continue;
        int bx = SS_GAME_OFFSET_X + (int)bullets[i].x;
        int by = SS_GAME_OFFSET_Y + (int)bullets[i].y;
        
        vga_set_color(bullets[i].color, VGA_BLCK);
        vga_putchr_at(bx, by, bullets[i].is_player_bullet ? '|' : '.');
    }
}

void spaceshooter_draw_enemies(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
    if (!enemies[i].active) continue;
        int ex = SS_GAME_OFFSET_X + (int)enemies[i].x;
        int ey = SS_GAME_OFFSET_Y + (int)enemies[i].y;
        
        vga_set_color(enemies[i].color, VGA_BLCK);
        vga_putchr_at(ex, ey, enemies[i].symbol);
    }
}

void spaceshooter_draw_powerups(void) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
    if (!powerups[i].active) continue;
        int px = SS_GAME_OFFSET_X + (int)powerups[i].x;
        int py = SS_GAME_OFFSET_Y + (int)powerups[i].y;
        
        vga_set_color(powerups[i].color, VGA_BLCK);
        vga_putchr_at(px, py, powerups[i].symbol);
    }
}

void spaceshooter_draw_explosions(void) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
    if (!explosions[i].active) continue;
        int ex = SS_GAME_OFFSET_X + (int)explosions[i].x;
        int ey = SS_GAME_OFFSET_Y + (int)explosions[i].y;
        
        vga_set_color(explosions[i].color, VGA_BLCK);
        
        if (explosions[i].frame < 4) {
            vga_putchr_at(ex, ey, '*');
        } else if (explosions[i].frame < 8) {
            vga_putchr_at(ex - 1, ey, '.');
            vga_putchr_at(ex, ey, 'O');
            vga_putchr_at(ex + 1, ey, '.');
        } else {
            vga_putchr_at(ex - 1, ey - 1, '.');
            vga_putchr_at(ex + 1, ey - 1, '.');
            vga_putchr_at(ex - 1, ey + 1, '.');
            vga_putchr_at(ex + 1, ey + 1, '.');
        }
    }
}

void spaceshooter_draw_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
        int px = SS_GAME_OFFSET_X + (int)particles[i].x;
        int py = SS_GAME_OFFSET_Y + (int)particles[i].y;
        
        vga_set_color(particles[i].color, VGA_BLCK);
        vga_putchr_at(px, py, particles[i].symbol);
    }
}

void spaceshooter_draw_stars(void) {
    for (int i = 0; i < MAX_STARS; i++) {
        int sx = SS_GAME_OFFSET_X + (int)stars[i].x;
        int sy = SS_GAME_OFFSET_Y + (int)stars[i].y;
        vga_set_color(stars[i].color, VGA_BLCK);
        vga_putchr_at(sx, sy, stars[i].symbol);
    }
}

void spaceshooter_draw_boss(void) {
    if (!boss.active) return;
    int bx = SS_GAME_OFFSET_X + (int)boss.x;
    int by = SS_GAME_OFFSET_Y + (int)boss.y;

    vga_set_color(boss.color, VGA_BLCK);

    vga_putchr_at(bx - 4, by, '/');
    vga_putchr_at(bx - 3, by, '-');
    vga_putchr_at(bx - 2, by, '-');
    vga_putchr_at(bx - 1, by, '-');
    vga_putchr_at(bx, by, 0x01);  
    vga_putchr_at(bx + 1, by, '-');
    vga_putchr_at(bx + 2, by, '-');
    vga_putchr_at(bx + 3, by, '-');
    vga_putchr_at(bx + 4, by, '\\');

    vga_putchr_at(bx - 3, by + 1, '|');
    vga_putchr_at(bx - 2, by + 1, 0xDB);
    vga_putchr_at(bx - 1, by + 1, 0xDB);
    vga_putchr_at(bx, by + 1, 0xDB);
    vga_putchr_at(bx + 1, by + 1, 0xDB);
    vga_putchr_at(bx + 2, by + 1, 0xDB);
    vga_putchr_at(bx + 3, by + 1, '|');

    vga_putchr_at(bx - 2, by + 2, '\\');
    vga_putchr_at(bx - 1, by + 2, 'V');
    vga_putchr_at(bx, by + 2, 'V');
    vga_putchr_at(bx + 1, by + 2, 'V');
    vga_putchr_at(bx + 2, by + 2, '/');

    spaceshooter_draw_health_bar(SS_GAME_OFFSET_X + 5, SS_GAME_OFFSET_Y + SS_GAME_HEIGHT + 1,
                                boss.health, boss.max_health, 50);
}

void spaceshooter_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("SPACE SHOOTER", 0);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_str[30];
    int pos = 0;
    const char* label = "Score: ";
    for (int i = 0; label[i]; i++) score_str[pos++] = label[i];

    int temp = player.score;
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
        vga_putchr_at(2 + i, SS_GAME_OFFSET_Y, score_str[i]);
    }

    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* lives_label = "Lives: ";
    int x = 2;
    for (int i = 0; lives_label[i]; i++) {
        vga_putchr_at(x++, SS_GAME_OFFSET_Y + 2, lives_label[i]);
    }
    for (int i = 0; i < player.lives; i++) {
        vga_putchr_at(x++, SS_GAME_OFFSET_Y + 2, 0x03);
    }

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char wave_str[20];
    pos = 0;
    const char* wave_label = "Wave: ";
    for (int i = 0; wave_label[i]; i++) wave_str[pos++] = wave_label[i];
    wave_str[pos++] = '0' + (wave_system.wave_number / 10);
    wave_str[pos++] = '0' + (wave_system.wave_number % 10);
    wave_str[pos] = '\0';

    for (int i = 0; wave_str[i]; i++) {
        vga_putchr_at(2 + i, SS_GAME_OFFSET_Y + 4, wave_str[i]);
    }

    spaceshooter_draw_health_bar(2, SS_GAME_OFFSET_Y + 6, player.health, player.max_health, 20);

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char weapon_str[20];
    pos = 0;
    const char* weapon_label = "WPN: ";
    for (int i = 0; weapon_label[i]; i++) weapon_str[pos++] = weapon_label[i];
    weapon_str[pos++] = '0' + player.weapon_level;
    weapon_str[pos] = '\0';

    for (int i = 0; weapon_str[i]; i++) {
        vga_putchr_at(2 + i, SS_GAME_OFFSET_Y + 8, weapon_str[i]);
    }

    int powerup_y = SS_GAME_OFFSET_Y + 10;
    if (player.shield_active) {
        vga_set_color(VGA_LBLUE, VGA_BLCK);
        const char* shield = "SHIELD";
        for (int i = 0; shield[i]; i++) {
            vga_putchr_at(2 + i, powerup_y++, shield[i]);
        }
    }
    if (player.rapid_fire) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        const char* rapid = "RAPID";
        for (int i = 0; rapid[i]; i++) {
            vga_putchr_at(2 + i, powerup_y++, rapid[i]);
        }
    }

    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls = "WASD/Arrows:Move | Space:Shoot | P:Pause";
    int len = 0;
    while (controls[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 24, controls[i]);
    }
}

void spaceshooter_draw_health_bar(int x, int y, int health, int max_health, int width) {
    int filled = (health * width) / max_health;
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    uint8_t color = VGA_LGREEN;
    if (health < max_health / 3) color = VGA_LRED;
    else if (health < max_health * 2 / 3) color = VGA_YELLOW;

    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_putchr_at(x - 1, y, '[');
    vga_putchr_at(x + width, y, ']');

    for (int i = 0; i < width; i++) {
        if (i < filled) {
            vga_set_color(color, VGA_BLCK);
            vga_putchr_at(x + i, y, 0xDB);
        } else {
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_putchr_at(x + i, y, 0xB0);
        }
    }
}

void spaceshooter_draw_pause_screen(void) {
    vga_draw_box_double(25, 10, 30, 5, VGA_YELLOW, VGA_BLCK);
    vga_fill_rect(26, 11, 28, 3, ' ', VGA_YELLOW, VGA_DGREY);
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* pause = "PAUSED";
    int len = 0;
    while (pause[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 11, pause[i]);
    }

    vga_set_color(VGA_WHITE, VGA_DGREY);
    const char* inst = "Press P to continue";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, inst[i]);
    }
}

void spaceshooter_draw_game_over(void) {
    vga_begin_batch();
    vga_clear();
    vga_draw_box_double(15, 6, 50, 12, VGA_RED, VGA_BLCK);

    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* go = "GAME OVER!";
    int len = 0;
    while (go[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 8, go[i]);
    }

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_str[40];
    int pos = 0;
    const char* label = "Final Score: ";
    for (int i = 0; label[i]; i++) score_str[pos++] = label[i];

    int temp = player.score;
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

    len = 0;
    while (score_str[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 11, score_str[i]);
    }

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char wave_str[40];
    pos = 0;
    const char* wave_label = "Wave Reached: ";
    for (int i = 0; wave_label[i]; i++) wave_str[pos++] = wave_label[i];
    wave_str[pos++] = '0' + (wave_system.wave_number / 10);
    wave_str[pos++] = '0' + (wave_system.wave_number % 10);
    wave_str[pos] = '\0';

    len = 0;
    while (wave_str[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, wave_str[i]);
    }

    if (player.score > high_score) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        const char* hs = "*** NEW HIGH SCORE! ***";
        len = 0;
        while (hs[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 15, hs[i]);
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

void spaceshooter_draw_level_complete(void) {
    vga_begin_batch();
    vga_clear();
    vga_draw_box_double(20, 8, 40, 10, VGA_GREEN, VGA_BLCK);

    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* complete = "BOSS DEFEATED!";
    int len = 0;
    while (complete[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, complete[i]);
    }

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* bonus = "+5000 Bonus Points!";
    len = 0;
    while (bonus[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 12, bonus[i]);
    }

    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst = "Press ENTER to continue";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, inst[i]);
    }
    vga_end_batch();
}

int spaceshooter_get_enemy_health(int type) {
    switch (type) {
        case ENEMY_BASIC: return 10;
        case ENEMY_FAST: return 5;
        case ENEMY_TANK: return 30;
        case ENEMY_ZIGZAG: return 15;
        case ENEMY_SHOOTER: return 20;
        case ENEMY_KAMIKAZE: return 8;
        case ENEMY_SPINNER: return 12;
        default: return 10;
    }
}
int spaceshooter_get_enemy_points(int type) {
    switch (type) {
        case ENEMY_BASIC: return 100;
        case ENEMY_FAST: return 150;
        case ENEMY_TANK: return 300;
        case ENEMY_ZIGZAG: return 200;
        case ENEMY_SHOOTER: return 250;
        case ENEMY_KAMIKAZE: return 180;
        case ENEMY_SPINNER: return 220;
        default: return 100;
    }
}
uint8_t spaceshooter_get_enemy_color(int type) {
    switch (type) {
        case ENEMY_BASIC: return VGA_LRED;
        case ENEMY_FAST: return VGA_YELLOW;
        case ENEMY_TANK: return VGA_DGREY;
        case ENEMY_ZIGZAG: return VGA_LMAGENTA;
        case ENEMY_SHOOTER: return VGA_RED;
        case ENEMY_KAMIKAZE: return VGA_LGREEN;
        case ENEMY_SPINNER: return VGA_LCYAN;
        default: return VGA_LRED;
    }
}
char spaceshooter_get_enemy_symbol(int type) {
    switch (type) {
        case ENEMY_BASIC: return 'V';
        case ENEMY_FAST: return 'v';
        case ENEMY_TANK: return 'H';
        case ENEMY_ZIGZAG: return 'W';
        case ENEMY_SHOOTER: return 'Y';
        case ENEMY_KAMIKAZE: return 'X';
        case ENEMY_SPINNER: return 'O';
        default: return 'V';
    }
}
void spaceshooter_save_high_score(void) {
    typedef struct {
        int magic_number;     
        int high_score;
        int version;           
        int checksum;
    } GameSpacesSaveData;
    
    GameSpacesSaveData save_data;
    save_data.magic_number = 0x45454545;  
    save_data.high_score = high_score;
    save_data.version = 1;
    
    save_data.checksum = save_data.magic_number + 
                         save_data.high_score + 
                         save_data.version;
    
    fat16_write_file(GAME_SPACES_SAVE_FILE, 
                     (const char*)&save_data, 
                     sizeof(GameSpacesSaveData));
}
void spaceshooter_load_high_score(void) {
    if (!fat16_file_exists(GAME_SPACES_SAVE_FILE)) {
        high_score = 0;
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(GAME_SPACES_SAVE_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        high_score = 0;
        return;
    }
    
    typedef struct {
        int magic_number;
        int high_score;
        int version;
        int checksum;
    } GameSpacesSaveData;
    
    if (file_size < sizeof(GameSpacesSaveData)) {
        high_score = 0;
        return;
    }
    
    GameSpacesSaveData* save_data = (GameSpacesSaveData*)file_content;
    
    if (save_data->magic_number != 0x45454545) {
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

void spaceshooter_game_run(void) {
    spaceshooter_game_init();
    while (1) {
        spaceshooter_handle_input();
        
        if (game_state == SHOOTER_EXIT) {
            break;
        }
        
        if (game_state == SHOOTER_MENU) {
            spaceshooter_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == SHOOTER_PLAYING || game_state == SHOOTER_BOSS_FIGHT) {
            spaceshooter_update_game();
            spaceshooter_draw_game();
            pit_delay_ms(16);  
        } else if (game_state == SHOOTER_PAUSED) {
            spaceshooter_draw_game();
            spaceshooter_draw_pause_screen();
            pit_delay_ms(50);
        } else if (game_state == SHOOTER_GAME_OVER) {
            spaceshooter_draw_game_over();
            game_state = SHOOTER_MENU;
        } else if (game_state == SHOOTER_LEVEL_COMPLETE) {
            spaceshooter_draw_level_complete();
            pit_delay_ms(50);
        }
    }
}

void spaceshooter_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}