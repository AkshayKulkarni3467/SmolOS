#include "sos_typeracer.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_memory.h"

static FallingWord words[MAX_WORDS];
static TypeParticle particles[MAX_PARTICLES];
static TypeExplosion explosions[MAX_EXPLOSIONS];
static GameSession session;

static TypeRacerState game_state;
static int menu_selection;
static int mode_selection;
static int difficulty_selection;
static int high_score;

static char input_buffer[MAX_INPUT_LENGTH];
static int input_length;
static int current_target_word;

static int screen_shake_intensity;
static int screen_shake_duration;

static uint32_t type_rand_seed = 42069;

// Word bank 
static const char* word_bank[WORD_BANK_SIZE] = {
    "vga", "int", "ptr", "key", "cpu", "idt", "gdt", "irq", "bit",
    "byte", "char", "void", "boot", "main", "call", "loop", "push",
    "pop", "mov", "jmp", "ret", "cli", "sti", "inb", "outb",
    "kernel", "memory", "string", "buffer", "screen", "cursor",
    "driver", "printf", "malloc", "struct", "static", "inline",
    "system", "header", "include", "return", "switch", "sizeof",
    "typedef", "extern", "global", "handler", "vector", "address",
    "interrupt", "scheduler", "allocator", "descriptor", "protected",
    "segmentation", "pagination", "exception", "keyboard", "initialize", "volatile", 
    "register","architecture", "initialization", "implementation", "configuration",
    "synchronization", "virtualization", "compatibility", "optimization",
    "while", "break", "const", "union", "enum", "signed", "unsigned",
    "continue", "default", "register", "volatile", "restrict",
    "stack", "heap", "queue", "mutex", "thread", "process", "fork",
    "exec", "pipe", "signal", "timer", "clock", "disk", "file",
    "inode", "dentry", "mount", "syscall", "user", "space", "mode",
    "port", "bus", "pci", "dma", "pic", "pit", "cmos", "rtc",
    "uart", "serial", "parallel", "floppy", "ide", "ata", "sata",
    "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
    "segment", "offset", "flag", "carry", "zero", "sign", "trap",
    "pointer", "array", "function", "variable", "constant", "macro",
    "define", "ifdef", "ifndef", "endif", "pragma", "inline",
    "assembly", "compiler", "linker", "loader", "debugger",
    "page", "frame", "table", "cache", "flush", "tlb", "mmu",
    "virtual", "physical", "mapped", "unmapped", "resident",
    "bootloader", "multiboot", "grub", "elf", "binary", "image",
    "sector", "cylinder", "track", "partition", "filesystem",
    "fat16", "fat32", "ext2", "ext3", "ext4", "ntfs",
    "lock", "unlock", "atomic", "barrier", "semaphore", "spinlock",
    "critical", "section", "deadlock", "livelock", "starvation",
    "maskable", "nonmaskable", "spurious", "nested", "latency",
    "priority", "preempt", "context", "switch", "save", "restore"
};

static int word_bank_count = 150; 

uint32_t typeracer_rand(void) {
    type_rand_seed = (type_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return type_rand_seed;
}

float typeracer_randf(void) {
    return (float)(typeracer_rand() % 1000) / 1000.0f;
}


int typeracer_string_length(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

int typeracer_string_compare(const char* s1, const char* s2, int n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i]) return 0;
        if (s1[i] == '\0') return 1;
    }
    return 1;
}

void typeracer_string_copy(char* dest, const char* src) {
    int i = 0;
    while (src[i] != '\0' && i < MAX_WORD_LENGTH - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}


void typeracer_game_init(void) {
    game_state = TYPE_MENU;
    menu_selection = 0;
    mode_selection = 0;
    difficulty_selection = 1;  
    high_score = 0;
    
    typeracer_load_high_score();
    typeracer_init_word_bank();
}

void typeracer_init_word_bank(void) {
    //TODO Add dynamic work bank?
}

void typeracer_reset_game(void) {
    for (int i = 0; i < MAX_WORDS; i++) {
        words[i].active = 0;
    }

    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = 0;
    }
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        explosions[i].active = 0;
    }

    session.stats.score = 0;
    session.stats.combo = 0;
    session.stats.max_combo = 0;
    session.stats.words_destroyed = 0;
    session.stats.total_chars_typed = 0;
    session.stats.correct_chars = 0;
    session.stats.incorrect_chars = 0;
    session.stats.lives = 3;
    session.stats.accuracy = 100.0f;
    session.stats.level = 1;
    session.stats.wpm = 0;

    session.combo.combo_count = 0;
    session.combo.multiplier = 1.0f;
    session.combo.last_destroy_time = 0;
    session.combo.combo_active = 0;

    switch (session.difficulty) {
        case DIFF_EASY:
            session.spawn_rate = 120;
            session.word_speed_multiplier = 1;
            session.stats.lives = 5;
            break;
        case DIFF_MEDIUM:
            session.spawn_rate = 90;
            session.word_speed_multiplier = 2;
            session.stats.lives = 3;
            break;
        case DIFF_HARD:
            session.spawn_rate = 60;
            session.word_speed_multiplier = 3;
            session.stats.lives = 2;
            break;
        case DIFF_INSANE:
            session.spawn_rate = 40;
            session.word_speed_multiplier = 4;
            session.stats.lives = 1;
            break;
    }

    switch (session.mode) {
        case MODE_CLASSIC:
            break;
        case MODE_SPEED_DEMON:
            session.spawn_rate /= 2;  
            session.word_speed_multiplier *= 2;
            break;
        case MODE_SURVIVAL:
            session.stats.lives = 1;  
            break;
        case MODE_ZEN:
            session.stats.lives = 999;  
            break;
    }

    input_length = 0;
    input_buffer[0] = '\0';
    current_target_word = -1;

    screen_shake_intensity = 0;
    screen_shake_duration = 0;

    session.start_time = pit_get_total_milliseconds();
    session.game_duration = 0;
}


const char* typeracer_get_random_word(void) {
    int index = typeracer_rand() % word_bank_count;
    return word_bank[index];
}

int typeracer_get_word_difficulty(const char* word) {
    int len = typeracer_string_length(word);
    
    if (len <= 4) return 1;      
    if (len <= 7) return 2;      
    if (len <= 10) return 3;     
    return 4;                     
}

uint8_t typeracer_get_word_color(int difficulty) {
    switch (difficulty) {
        case 1: return VGA_LGREEN;
        case 2: return VGA_YELLOW;
        case 3: return VGA_LRED;
        case 4: return VGA_LMAGENTA;
        default: return VGA_WHITE;
    }
}


void typeracer_update_game(void) {
    static int spawn_timer = 0;

    session.game_duration = pit_get_total_milliseconds() - session.start_time;

    spawn_timer++;
    if (spawn_timer >= session.spawn_rate) {
        spawn_timer = 0;
        typeracer_spawn_word();
    }

    if (session.stats.words_destroyed > 0 && session.stats.words_destroyed % 10 == 0) {
        if (session.spawn_rate > 20) {
            session.spawn_rate -= 2;
        }
        session.stats.level = 1 + (session.stats.words_destroyed / 10);
    }
    
    typeracer_update_words();
    typeracer_update_particles();
    typeracer_update_explosions();
    typeracer_update_combo();
    typeracer_update_wpm();

    if (screen_shake_duration > 0) {
        screen_shake_duration--;
        if (screen_shake_duration == 0) {
            screen_shake_intensity = 0;
        }
    }
}

void typeracer_update_words(void) {
    for (int i = 0; i < MAX_WORDS; i++) {
        if (!words[i].active) continue;
        
        words[i].x += words[i].vx;
        words[i].y += words[i].vy;

        if (words[i].y >= GAME_HEIGHT - 1) {
            words[i].active = 0;

            if (session.mode != MODE_ZEN) {
                typeracer_lose_life();
            }

            session.combo.combo_count = 0;
            session.combo.multiplier = 1.0f;
            session.combo.combo_active = 0;
        }
    }
}

void typeracer_update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        particles[i].x += particles[i].vx;
        particles[i].y += particles[i].vy;
        particles[i].lifetime--;
        
        if (particles[i].lifetime <= 0) {
            particles[i].active = 0;
        }
    }
}

void typeracer_update_explosions(void) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;
        
        explosions[i].frame++;
        if (explosions[i].frame >= explosions[i].max_frames) {
            explosions[i].active = 0;
        }
    }
}

void typeracer_update_combo(void) {
    uint32_t current_time = pit_get_total_milliseconds();

    if (session.combo.combo_active && 
        current_time - session.combo.last_destroy_time > 1500) {
        session.combo.combo_count = 0;
        session.combo.multiplier = 1.0f;
        session.combo.combo_active = 0;
    }
}

void typeracer_update_wpm(void) {
    uint32_t elapsed_seconds = session.game_duration / 1000;
    if (elapsed_seconds > 0) {
        int words = session.stats.correct_chars / 5;
        session.stats.wpm = (words * 60) / elapsed_seconds;
    }
}


void typeracer_spawn_word(void) {
    for (int i = 0; i < MAX_WORDS; i++) {
        if (!words[i].active) {
            const char* new_word = typeracer_get_random_word();
            typeracer_string_copy(words[i].text, new_word);
            words[i].length = typeracer_string_length(new_word);
            words[i].x = typeracer_randf() * (GAME_WIDTH - words[i].length - 2);
            words[i].y = 0;
            words[i].vx = 0;
            words[i].vy = 0.05f + (typeracer_randf() * 0.05f * session.word_speed_multiplier);
            words[i].active = 1;
            words[i].typed_chars = 0;
            words[i].is_targeted = 0;
            
            int difficulty = typeracer_get_word_difficulty(new_word);
            words[i].color = typeracer_get_word_color(difficulty);
            words[i].points = difficulty * 100;
            
            break;
        }
    }
}

void typeracer_destroy_word(int index) {
    if (index < 0 || index >= MAX_WORDS || !words[index].active) return;

    int points = words[index].points * session.combo.multiplier;
    session.stats.score += points;
    session.stats.words_destroyed++;

    session.combo.combo_count++;
    session.combo.last_destroy_time = pit_get_total_milliseconds();
    session.combo.combo_active = 1;

    if (session.combo.combo_count >= 5) {
        session.combo.multiplier = 2.0f;
    }
    if (session.combo.combo_count >= 10) {
        session.combo.multiplier = 3.0f;
    }
    if (session.combo.combo_count >= 20) {
        session.combo.multiplier = 4.0f;
    }

    if (session.combo.combo_count > session.stats.max_combo) {
        session.stats.max_combo = session.combo.combo_count;
    }

    typeracer_spawn_explosion(words[index].x + words[index].length / 2, 
                             words[index].y, words[index].color);
    typeracer_spawn_particles(words[index].x + words[index].length / 2,
                             words[index].y, 15, words[index].color);

    if (session.combo.combo_count >= 10) {
        screen_shake_intensity = 2;
        screen_shake_duration = 5;
    }
    
    words[index].active = 0;

    typeracer_clear_input();
}

void typeracer_process_input(char c) {
    if (c == '\b') {
        if (input_length > 0) {
            input_length--;
            input_buffer[input_length] = '\0';

            if (current_target_word >= 0) {
                words[current_target_word].typed_chars--;
                if (words[current_target_word].typed_chars < 0) {
                    words[current_target_word].typed_chars = 0;
                }
            }
        }
        return;
    }
    
    if (c < 32 || c > 126) return;  
    
    if (input_length >= MAX_INPUT_LENGTH - 1) return;

    input_buffer[input_length] = c;
    input_length++;
    input_buffer[input_length] = '\0';
    
    session.stats.total_chars_typed++;

    int match = typeracer_find_matching_word();
    
    if (match >= 0) {
        session.stats.correct_chars++;
        words[match].typed_chars = input_length;
        words[match].is_targeted = 1;
        current_target_word = match;

        if (input_length == words[match].length) {
            typeracer_destroy_word(match);
            current_target_word = -1;
        }
    } else {
        session.stats.incorrect_chars++;

        typeracer_clear_input();
        session.combo.combo_count = 0;
        session.combo.multiplier = 1.0f;
        session.combo.combo_active = 0;
        
        if (current_target_word >= 0) {
            words[current_target_word].typed_chars = 0;
            words[current_target_word].is_targeted = 0;
            current_target_word = -1;
        }
    }

    if (session.stats.total_chars_typed > 0) {
        session.stats.accuracy = (float)session.stats.correct_chars / 
                                 (float)session.stats.total_chars_typed * 100.0f;
    }
}

void typeracer_clear_input(void) {
    input_length = 0;
    input_buffer[0] = '\0';
    
    if (current_target_word >= 0) {
        words[current_target_word].is_targeted = 0;
        current_target_word = -1;
    }
}

int typeracer_find_matching_word(void) {
    if (current_target_word >= 0 && words[current_target_word].active) {
        if (typeracer_string_compare(input_buffer, words[current_target_word].text, input_length)) {
            return current_target_word;
        } else {
            return -1;
        }
    }

    int best_match = -1;
    float best_y = -1;
    
    for (int i = 0; i < MAX_WORDS; i++) {
        if (!words[i].active) continue;
        if (words[i].is_targeted) continue;
        
        if (typeracer_string_compare(input_buffer, words[i].text, input_length)) {
            if (best_match == -1 || words[i].y > best_y) {
                best_match = i;
                best_y = words[i].y;
            }
        }
    }
    
    return best_match;
}

void typeracer_lose_life(void) {
    session.stats.lives--;
    
    typeracer_screen_shake();
    
    if (session.stats.lives <= 0) {
        game_state = TYPE_GAME_OVER;
        
        if (session.stats.score > high_score) {
            high_score = session.stats.score;
            typeracer_save_high_score();
        }
    }
}


void typeracer_spawn_particles(float x, float y, int count, uint8_t color) {
    int spawned = 0;
    for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
        if (!particles[i].active) {
            particles[i].active = 1;
            particles[i].x = x;
            particles[i].y = y;
            particles[i].vx = (typeracer_randf() - 0.5f) * 0.8f;
            particles[i].vy = (typeracer_randf() - 0.5f) * 0.8f;
            particles[i].lifetime = 15 + typeracer_rand() % 20;
            particles[i].color = color;
            particles[i].symbol = (typeracer_rand() % 3 == 0) ? '*' : '.';
            spawned++;
        }
    }
}

void typeracer_spawn_explosion(float x, float y, uint8_t color) {
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) {
            explosions[i].active = 1;
            explosions[i].x = x;
            explosions[i].y = y;
            explosions[i].frame = 0;
            explosions[i].max_frames = 10;
            explosions[i].color = color;
            break;
        }
    }
}

void typeracer_screen_shake(void) {
    screen_shake_intensity = 3;
    screen_shake_duration = 8;
}


void typeracer_handle_input(void) {
    keyboard_poll();
    
    if (game_state == TYPE_MENU) {
        if (!has_key()) return;
        char c = get_char();
        
        if (c == 0x11) {  
            menu_selection--;
            if (menu_selection < 0) menu_selection = 3;
        } else if (c == 0x12) {  
            menu_selection++;
            if (menu_selection > 3) menu_selection = 0;
        } else if (c == 0x13) {  
            if (menu_selection == 1) {
                mode_selection--;
                if (mode_selection < 0) mode_selection = 3;
            } else if (menu_selection == 2) {
                difficulty_selection--;
                if (difficulty_selection < 0) difficulty_selection = 3;
            }
        } else if (c == 0x14) { 
            if (menu_selection == 1) {
                mode_selection++;
                if (mode_selection > 3) mode_selection = 0;
            } else if (menu_selection == 2) {
                difficulty_selection++;
                if (difficulty_selection > 3) difficulty_selection = 0;
            }
        } else if (c == '\n') {  
            if (menu_selection == 0) {
                session.mode = mode_selection;
                session.difficulty = difficulty_selection;
                typeracer_reset_game();
                game_state = TYPE_PLAYING;
            } else if (menu_selection == 3) {
                game_state = TYPE_EXIT;
            }
        } else if (c == 27) { 
            game_state = TYPE_EXIT;
        }
    } else if (game_state == TYPE_PLAYING) {
        while (has_key()) {
            char c = get_char();
            
            if (c == 27) {  
                game_state = TYPE_MENU;
                break;
            } else {
                typeracer_process_input(c);
            }
        }
    } else if (game_state == TYPE_PAUSED) {
        if (!has_key()) return;
        char c = get_char();
        
        if (c == 'p' || c == 'P' || c == ' ' || c == '\n') {
            game_state = TYPE_PLAYING;
        } else if (c == 27) {
            game_state = TYPE_MENU;
        }
    } else if (game_state == TYPE_STATS_SCREEN) {
        if (!has_key()) return;
        get_char();
        game_state = TYPE_MENU;
    }
}


void typeracer_draw_menu(void) {
    vga_clear();

    const char* title[] = {
        " _____ _   _ ____  _____      ____      _    ____ _____ ____  ",
        "|_   _| \\ | |  _ \\| ____|    |  _ \\    / \\  / ___| ____|  _ \\ ",
        "  | | |  \\| | |_) |  _| _____| |_) |  / _ \\| |   |  _| | |_) |",
        "  | | | |\\  |  __/| |__|_____|  _ <  / ___ \\ |___| |___|  _ < ",
        "  |_| |_| \\_|_|   |_____|    |_| \\_\\/_/   \\_\\____|_____|_| \\_\\"
    };
    
    uint8_t colors[] = {VGA_LCYAN, VGA_LBLUE, VGA_LMAGENTA, VGA_LRED, VGA_YELLOW};
    
    for (int i = 0; i < 5; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        vga_set_color(colors[i], VGA_BLCK);
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 2 + i, title[i][j]);
        }
    }

    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* subtitle = "Type to Destroy! - Kernel Edition";
    int len = 0;
    while (subtitle[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 8, subtitle[i]);
    }

    const char* options[] = {
        "Start Game",
        "Mode",
        "Difficulty",
        "Exit"
    };
    
    const char* modes[] = {"Classic", "Speed Demon", "Survival", "Zen"};
    const char* difficulties[] = {"Easy", "Medium", "Hard", "Insane"};
    
    for (int i = 0; i < 4; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(20, 12 + i * 2, 40, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 25;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 12 + i * 2, options[i][j]);
        }

        if (i == 1) {
            vga_set_color(VGA_LGREEN, selected ? VGA_BLUE : VGA_BLCK);
            const char* mode_text = modes[mode_selection];
            x += 2;
            for (int j = 0; mode_text[j]; j++) {
                vga_putchr_at(x++, 12 + i * 2, mode_text[j]);
            }
        } else if (i == 2) {
            vga_set_color(VGA_LRED, selected ? VGA_BLUE : VGA_BLCK);
            const char* diff_text = difficulties[difficulty_selection];
            x += 2;
            for (int j = 0; diff_text[j]; j++) {
                vga_putchr_at(x++, 12 + i * 2, diff_text[j]);
            }
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
    
    len = 0;
    while (hs[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 21, hs[i]);
    }

    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* inst = "Use ARROWS to navigate, ENTER to select, ESC to exit";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 23, inst[i]);
    }
}

void typeracer_draw_game(void) {
    vga_begin_batch();
    vga_clear();
    
    typeracer_draw_ui();
    typeracer_draw_field();
    typeracer_draw_words();
    typeracer_draw_explosions();
    typeracer_draw_particles();
    typeracer_draw_input_area();
    typeracer_draw_combo_meter();
    
    vga_end_batch();
}

void typeracer_draw_field(void) {
    int shake_x = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    int shake_y = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    
    vga_draw_box_double(GAME_OFFSET_X + shake_x, GAME_OFFSET_Y + shake_y,
                        GAME_WIDTH, GAME_HEIGHT, VGA_CYAN, VGA_BLCK);
}

void typeracer_draw_words(void) {
    int shake_x = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    int shake_y = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    
    for (int i = 0; i < MAX_WORDS; i++) {
        if (!words[i].active) continue;
        
        int wx = GAME_OFFSET_X + (int)words[i].x + shake_x;
        int wy = GAME_OFFSET_Y + (int)words[i].y + shake_y;

        for (int j = 0; j < words[i].length; j++) {
            uint8_t color;
            
            if (j < words[i].typed_chars) {
                color = VGA_LGREEN;
            } else if (words[i].is_targeted && j == words[i].typed_chars) {
                color = VGA_YELLOW;
            } else {
                color = words[i].color;
            }
            
            vga_set_color(color, VGA_BLCK);
            vga_putchr_at(wx + j, wy, words[i].text[j]);
        }
    }
}

void typeracer_draw_input_area(void) {
    vga_draw_box_single(GAME_OFFSET_X, INPUT_Y, GAME_WIDTH, 2, VGA_YELLOW, VGA_BLCK);
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* prompt = "Type: ";
    int x = GAME_OFFSET_X + 2;
    for (int i = 0; prompt[i]; i++) {
        vga_putchr_at(x++, INPUT_Y + 1, prompt[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    for (int i = 0; i < input_length; i++) {
        vga_putchr_at(x + i, INPUT_Y + 1, input_buffer[i]);
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(x + input_length, INPUT_Y + 1, '_');
}

void typeracer_draw_particles(void) {
    int shake_x = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    int shake_y = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        
        int px = GAME_OFFSET_X + (int)particles[i].x + shake_x;
        int py = GAME_OFFSET_Y + (int)particles[i].y + shake_y;
        
        vga_set_color(particles[i].color, VGA_BLCK);
        vga_putchr_at(px, py, particles[i].symbol);
    }
}

void typeracer_draw_explosions(void) {
    int shake_x = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    int shake_y = (screen_shake_duration > 0) ? (typeracer_rand() % 3) - 1 : 0;
    
    for (int i = 0; i < MAX_EXPLOSIONS; i++) {
        if (!explosions[i].active) continue;
        
        int ex = GAME_OFFSET_X + (int)explosions[i].x + shake_x;
        int ey = GAME_OFFSET_Y + (int)explosions[i].y + shake_y;
        
        vga_set_color(explosions[i].color, VGA_BLCK);
        
        if (explosions[i].frame < 3) {
            vga_putchr_at(ex, ey, '*');
        } else if (explosions[i].frame < 6) {
            vga_putchr_at(ex - 1, ey, '*');
            vga_putchr_at(ex, ey, 'O');
            vga_putchr_at(ex + 1, ey, '*');
        } else {
            vga_putchr_at(ex - 1, ey - 1, '.');
            vga_putchr_at(ex + 1, ey - 1, '.');
            vga_putchr_at(ex - 1, ey + 1, '.');
            vga_putchr_at(ex + 1, ey + 1, '.');
        }
    }
}

void typeracer_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== TYPE-RACER ===", 0);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char score_str[30];
    int pos = 0;
    const char* label = "Score: ";
    for (int i = 0; label[i]; i++) score_str[pos++] = label[i];
    
    int temp = session.stats.score;
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
        vga_putchr_at(2 + i, 1, score_str[i]);
    }
    

    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* lives_label = "Lives: ";
    int x = 2;
    for (int i = 0; lives_label[i]; i++) {
        vga_putchr_at(x++, 2, lives_label[i]);
    }
    
    if (session.stats.lives < 10) {
        for (int i = 0; i < session.stats.lives; i++) {
            vga_putchr_at(x++, 2, 0x03);  
        }
    } else {
        char lives_num[5];
        pos = 0;
        temp = session.stats.lives;
        if (temp == 0) {
            lives_num[pos++] = '0';
        } else {
            char digits[10];
            int digit_count = 0;
            while (temp > 0) {
                digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int i = digit_count - 1; i >= 0; i--) {
                lives_num[pos++] = digits[i];
            }
        }
        lives_num[pos] = '\0';
        for (int i = 0; lives_num[i]; i++) {
            vga_putchr_at(x++, 2, lives_num[i]);
        }
    }

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char level_str[20];
    pos = 0;
    const char* level_label = "Level: ";
    for (int i = 0; level_label[i]; i++) level_str[pos++] = level_label[i];
    level_str[pos++] = '0' + (session.stats.level / 10);
    level_str[pos++] = '0' + (session.stats.level % 10);
    level_str[pos] = '\0';
    
    for (int i = 0; level_str[i]; i++) {
        vga_putchr_at(30 + i, 1, level_str[i]);
    }

    vga_set_color(VGA_LGREEN, VGA_BLCK);
    char wpm_str[20];
    pos = 0;
    const char* wpm_label = "WPM: ";
    for (int i = 0; wpm_label[i]; i++) wpm_str[pos++] = wpm_label[i];
    
    temp = session.stats.wpm;
    if (temp == 0) {
        wpm_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            wpm_str[pos++] = digits[i];
        }
    }
    wpm_str[pos] = '\0';
    
    for (int i = 0; wpm_str[i]; i++) {
        vga_putchr_at(30 + i, 2, wpm_str[i]);
    }

    vga_set_color(VGA_LMAGENTA, VGA_BLCK);
    typeracer_draw_accuracy_bar(50, 1, session.stats.accuracy, 25);

    vga_set_color(VGA_WHITE, VGA_BLCK);
    char words_str[30];
    pos = 0;
    const char* words_label = "Words: ";
    for (int i = 0; words_label[i]; i++) words_str[pos++] = words_label[i];
    
    temp = session.stats.words_destroyed;
    if (temp == 0) {
        words_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            words_str[pos++] = digits[i];
        }
    }
    words_str[pos] = '\0';
    
    for (int i = 0; words_str[i]; i++) {
        vga_putchr_at(50 + i, 2, words_str[i]);
    }
}

void typeracer_draw_combo_meter(void) {
    if (session.combo.combo_count <= 0) return;
    
    int combo_x = GAME_OFFSET_X + GAME_WIDTH + 2;
    int combo_y = GAME_OFFSET_Y + 2;

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* combo_label = "COMBO!";
    for (int i = 0; combo_label[i]; i++) {
        vga_putchr_at(combo_x, combo_y + i, combo_label[i]);
    }

    vga_set_color(VGA_LRED, VGA_BLCK);
    char combo_str[10];
    int pos = 0;
    int temp = session.combo.combo_count;
    if (temp == 0) {
        combo_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            combo_str[pos++] = digits[i];
        }
    }
    combo_str[pos++] = 'x';
    combo_str[pos] = '\0';
    
    for (int i = 0; combo_str[i]; i++) {
        vga_putchr_at(combo_x, combo_y + 7 + i, combo_str[i]);
    }

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    char mult_str[10];
    pos = 0;
    temp = (int)session.combo.multiplier;
    if (temp == 0) {
        mult_str[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            mult_str[pos++] = digits[i];
        }
    }
    mult_str[pos++] = 'x';
    mult_str[pos] = '\0';
    
    for (int i = 0; mult_str[i]; i++) {
        vga_putchr_at(combo_x, combo_y + 12 + i, mult_str[i]);
    }
}

void typeracer_draw_accuracy_bar(int x, int y, float accuracy, int width) {
    vga_set_color(VGA_DGREY, VGA_BLCK);
    const char* acc_label = "ACC: ";
    for (int i = 0; acc_label[i]; i++) {
        vga_putchr_at(x + i, y, acc_label[i]);
    }
    
    int start_x = x + 5;
    int filled = (int)((accuracy / 100.0f) * width);
    if (filled > width) filled = width;
    
    uint8_t color = VGA_LGREEN;
    if (accuracy < 70.0f) color = VGA_LRED;
    else if (accuracy < 85.0f) color = VGA_YELLOW;
    
    vga_putchr_at(start_x - 1, y, '[');
    vga_putchr_at(start_x + width, y, ']');
    
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            vga_set_color(color, VGA_BLCK);
            vga_putchr_at(start_x + i, y, 0xDB);
        } else {
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_putchr_at(start_x + i, y, 0xB0);
        }
    }
}

void typeracer_draw_pause_screen(void) {
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

void typeracer_draw_game_over(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(10, 4, 60, 16, VGA_RED, VGA_BLCK);
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    const char* go = "GAME OVER!";
    int len = 0;
    while (go[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 6, go[i]);
    }

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    char stats[50];
    int pos;

    pos = 0;
    const char* score_label = "Final Score: ";
    for (int i = 0; score_label[i]; i++) stats[pos++] = score_label[i];
    int temp = session.stats.score;
    if (temp == 0) {
        stats[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stats[pos++] = digits[i];
        }
    }
    stats[pos] = '\0';
    len = 0;
    while (stats[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 9, stats[i]);
    }

    vga_set_color(VGA_LCYAN, VGA_BLCK);
    pos = 0;
    const char* words_label = "Words Destroyed: ";
    for (int i = 0; words_label[i]; i++) stats[pos++] = words_label[i];
    temp = session.stats.words_destroyed;
    if (temp == 0) {
        stats[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stats[pos++] = digits[i];
        }
    }
    stats[pos] = '\0';
    len = 0;
    while (stats[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 11, stats[i]);
    }

    vga_set_color(VGA_LMAGENTA, VGA_BLCK);
    pos = 0;
    const char* combo_label = "Max Combo: ";
    for (int i = 0; combo_label[i]; i++) stats[pos++] = combo_label[i];
    temp = session.stats.max_combo;
    if (temp == 0) {
        stats[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stats[pos++] = digits[i];
        }
    }
    stats[pos++] = 'x';
    stats[pos] = '\0';
    len = 0;
    while (stats[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 13, stats[i]);
    }

    vga_set_color(VGA_LGREEN, VGA_BLCK);
    pos = 0;
    const char* wpm_label = "Words Per Minute: ";
    for (int i = 0; wpm_label[i]; i++) stats[pos++] = wpm_label[i];
    temp = session.stats.wpm;
    if (temp == 0) {
        stats[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            stats[pos++] = digits[i];
        }
    }
    stats[pos] = '\0';
    len = 0;
    while (stats[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, stats[i]);
    }

    typeracer_draw_accuracy_bar(25, 17, session.stats.accuracy, 30);

    if (session.stats.score > high_score) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        const char* hs = "*** NEW HIGH SCORE! ***";
        len = 0;
        while (hs[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 19, hs[i]);
        }
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    const char* inst = "Press any key to return to menu";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 22, inst[i]);
    }
    
    vga_end_batch();
    
    wait_for_char();
}


void typeracer_save_high_score(void) {
    // TODO: Save to FAT16
}

void typeracer_load_high_score(void) {
    high_score = 0;
}


void typeracer_game_run(void) {
    typeracer_game_init();
    
    while (1) {
        typeracer_handle_input();
        
        if (game_state == TYPE_EXIT) {
            break;
        }
        
        if (game_state == TYPE_MENU) {
            typeracer_draw_menu();
            pit_delay_ms(50);
        } else if (game_state == TYPE_PLAYING) {
            typeracer_update_game();
            typeracer_draw_game();
            pit_delay_ms(16);  
        } else if (game_state == TYPE_PAUSED) {
            typeracer_draw_game();
            typeracer_draw_pause_screen();
            pit_delay_ms(50);
        } else if (game_state == TYPE_GAME_OVER) {
            typeracer_draw_game_over();
            game_state = TYPE_MENU;
        }
    }
}

void typeracer_game_cleanup(void) {
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}