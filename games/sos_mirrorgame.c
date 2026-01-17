#include "sos_mirrorgame.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_mouse.h"
#include "sos_pit.h"
#include "sos_memory.h"

static MirrorState game_state;
static Level current_level;
static Mirror mirrors[MAX_MIRRORS];
static int mirror_count;
static BeamPath beam_path;
static int current_level_num;
static int menu_selection;
static Mirror* dragging_mirror;
static int drag_offset_x, drag_offset_y;
static int selected_palette_item;
static int moves_made;
static int best_moves[MAX_LEVELS];
static uint32_t level_start_time;
static int show_beam_animation;
static int animation_frame;
static uint32_t proc_rand_seed;

static void mirror_init_levels(void);

static uint32_t proc_rand(void) {
    proc_rand_seed = (proc_rand_seed * 1103515245 + 12345) & 0x7FFFFFFF;
    return proc_rand_seed;
}

void mirror_init(void) {
    game_state = MIRROR_MENU;
    current_level_num = 0;
    menu_selection = 0;
    dragging_mirror = 0;
    selected_palette_item = 0;
    show_beam_animation = 1;
    animation_frame = 0;
    proc_rand_seed = pit_get_ticks() * 12345;
    
    for (int i = 0; i < MAX_LEVELS; i++) {
        best_moves[i] = 999;
    }
    
    mouse_show_cursor();
    mouse_set_cursor_char(0x10);
    
    mirror_init_levels();
}

void mirror_init_levels(void) {
    //TODO Implement levels
}

void mirror_load_level(int level_num) {
    current_level_num = level_num;
    mirror_count = 0;
    moves_made = 0;
    dragging_mirror = 0;
    beam_path.count = 0;
    beam_path.hits_target = 0;
    level_start_time = pit_get_total_milliseconds();
    
    for (int i = 0; i < MAX_MIRRORS; i++) {
        mirrors[i].is_placed = 0;
        mirrors[i].is_dragging = 0;
    }
    
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            current_level.grid[y][x] = MIRROR_NONE;
        }
    }
    
    current_level.is_procedural = 0;
    
    if (level_num == 0) {
        const char* name = "Tutorial: First Reflection";
        int i = 0;
        while (name[i] && i < 39) {
            current_level.name[i] = name[i];
            i++;
        }
        current_level.name[i] = '\0';
        
        current_level.laser_x = 2;
        current_level.laser_y = 2;
        current_level.laser_dir = MG_DIR_RIGHT;
        current_level.target_x = 32;
        current_level.target_y = 15;
        current_level.max_mirrors = 3;
        
        current_level.available_mirrors[0] = 2;
        current_level.available_mirrors[1] = 2;
        current_level.available_mirrors[2] = 0;
        current_level.available_mirrors[3] = 0;
        current_level.available_mirrors[4] = 0;
        
        for (int x = 0; x < MAZE_WIDTH; x++) {
            current_level.grid[0][x] = WALL;
            current_level.grid[MAZE_HEIGHT-1][x] = WALL;
        }
        for (int y = 0; y < MAZE_HEIGHT; y++) {
            current_level.grid[y][0] = WALL;
            current_level.grid[y][MAZE_WIDTH-1] = WALL;
        }
        
        current_level.grid[current_level.laser_y][current_level.laser_x] = LASER_SOURCE;
        current_level.grid[current_level.target_y][current_level.target_x] = TARGET;
        
    } else if (level_num == 1) {
        const char* name = "The Corner Shot";
        int i = 0;
        while (name[i] && i < 39) {
            current_level.name[i] = name[i];
            i++;
        }
        current_level.name[i] = '\0';
        
        current_level.laser_x = 2;
        current_level.laser_y = 9;
        current_level.laser_dir = MG_DIR_RIGHT;
        current_level.target_x = 17;
        current_level.target_y = 2;
        current_level.max_mirrors = 2;
        
        current_level.available_mirrors[0] = 1;
        current_level.available_mirrors[1] = 1;
        current_level.available_mirrors[2] = 0;
        current_level.available_mirrors[3] = 0;
        current_level.available_mirrors[4] = 0;
        
        for (int x = 0; x < MAZE_WIDTH; x++) {
            current_level.grid[0][x] = WALL;
            current_level.grid[MAZE_HEIGHT-1][x] = WALL;
        }
        for (int y = 0; y < MAZE_HEIGHT; y++) {
            current_level.grid[y][0] = WALL;
            current_level.grid[y][MAZE_WIDTH-1] = WALL;
        }
        
        for (int x = 10; x < 25; x++) {
            current_level.grid[9][x] = WALL;
        }
        
        current_level.grid[current_level.laser_y][current_level.laser_x] = LASER_SOURCE;
        current_level.grid[current_level.target_y][current_level.target_x] = TARGET;
        
    } else if (level_num == 2) {
        const char* name = "The Maze Runner";
        int i = 0;
        while (name[i] && i < 39) {
            current_level.name[i] = name[i];
            i++;
        }
        current_level.name[i] = '\0';
        
        current_level.laser_x = 2;
        current_level.laser_y = 2;
        current_level.laser_dir = MG_DIR_DOWN;
        current_level.target_x = 32;
        current_level.target_y = 15;
        current_level.max_mirrors = 5;
        
        current_level.available_mirrors[0] = 2;
        current_level.available_mirrors[1] = 2;
        current_level.available_mirrors[2] = 1;
        current_level.available_mirrors[3] = 0;
        current_level.available_mirrors[4] = 0;
        
        for (int x = 0; x < MAZE_WIDTH; x++) {
            current_level.grid[0][x] = WALL;
            current_level.grid[MAZE_HEIGHT-1][x] = WALL;
        }
        for (int y = 0; y < MAZE_HEIGHT; y++) {
            current_level.grid[y][0] = WALL;
            current_level.grid[y][MAZE_WIDTH-1] = WALL;
        }
        
        for (int y = 3; y < 12; y++) {
            current_level.grid[y][10] = WALL;
        }
        for (int x = 10; x < 25; x++) {
            current_level.grid[6][x] = WALL;
        }
        for (int y = 6; y < 15; y++) {
            current_level.grid[y][24] = WALL;
        }
        
        current_level.grid[current_level.laser_y][current_level.laser_x] = LASER_SOURCE;
        current_level.grid[current_level.target_y][current_level.target_x] = TARGET;
    } else {
        int difficulty = level_num - 2;
        mirror_generate_procedural_level(difficulty);
    }
    
    mirror_calculate_beam();
}

void mirror_reset_level(void) {
    mirror_load_level(current_level_num);
}

MG_Direction mirror_reflect(MG_Direction incoming, CellType mirror_type) {
    if (mirror_type == MIRROR_FORWARD_SLASH) {
        switch (incoming) {
            case MG_DIR_RIGHT: return MG_DIR_UP;
            case MG_DIR_DOWN: return MG_DIR_LEFT;
            case MG_DIR_LEFT: return MG_DIR_DOWN;
            case MG_DIR_UP: return MG_DIR_RIGHT;
            default: return MG_DIR_NONE;
        }
    } else if (mirror_type == MIRROR_BACKSLASH) {
        switch (incoming) {
            case MG_DIR_RIGHT: return MG_DIR_DOWN;
            case MG_DIR_DOWN: return MG_DIR_RIGHT;
            case MG_DIR_LEFT: return MG_DIR_UP;
            case MG_DIR_UP: return MG_DIR_LEFT;
            default: return MG_DIR_NONE;
        }
    } else if (mirror_type == MIRROR_HORIZONTAL) {
        switch (incoming) {
            case MG_DIR_UP: return MG_DIR_DOWN;
            case MG_DIR_DOWN: return MG_DIR_UP;
            default: return incoming;
        }
    } else if (mirror_type == MIRROR_VERTICAL) {
        switch (incoming) {
            case MG_DIR_LEFT: return MG_DIR_RIGHT;
            case MG_DIR_RIGHT: return MG_DIR_LEFT;
            default: return incoming;
        }
    }
    
    return MG_DIR_NONE;
}

void mirror_calculate_beam(void) {
    beam_path.count = 0;
    beam_path.hits_target = 0;
    
    int x = current_level.laser_x;
    int y = current_level.laser_y;
    MG_Direction dir = current_level.laser_dir;
    
    int max_iterations = MAX_BEAM_POINTS - 1;
    int iteration = 0;
    
    while (iteration < max_iterations) {
        beam_path.points[beam_path.count].x = x;
        beam_path.points[beam_path.count].y = y;
        beam_path.points[beam_path.count].dir = dir;
        beam_path.count++;
        
        switch (dir) {
            case MG_DIR_RIGHT: x++; break;
            case MG_DIR_LEFT: x--; break;
            case MG_DIR_DOWN: y++; break;
            case MG_DIR_UP: y--; break;
            default: return;
        }
        
        if (x < 0 || x >= MAZE_WIDTH || y < 0 || y >= MAZE_HEIGHT) {
            break;
        }
        
        CellType cell = current_level.grid[y][x];
        
        if (cell == WALL) {
            break;
        }
        
        if (cell == TARGET) {
            beam_path.points[beam_path.count].x = x;
            beam_path.points[beam_path.count].y = y;
            beam_path.points[beam_path.count].dir = dir;
            beam_path.count++;
            beam_path.hits_target = 1;
            break;
        }
        
        Mirror* mirror = mirror_get_at(x, y);
        if (mirror && mirror->is_placed && !mirror->is_dragging) {
            MG_Direction new_dir = mirror_reflect(dir, mirror->type);
            if (new_dir == MG_DIR_NONE) {
                break;
            }
            dir = new_dir;
        }
        
        iteration++;
    }
}

Mirror* mirror_get_at(int x, int y) {
    for (int i = 0; i < mirror_count; i++) {
        if (mirrors[i].is_placed && mirrors[i].x == x && mirrors[i].y == y) {
            return &mirrors[i];
        }
    }
    return 0;
}

void mirror_place_mirror(int x, int y, CellType type) {
    if (x < 1 || x >= MAZE_WIDTH - 1 || y < 1 || y >= MAZE_HEIGHT - 1) {
        return;
    }
    
    if (current_level.grid[y][x] != MIRROR_NONE) {
        return;
    }
    
    Mirror* existing = mirror_get_at(x, y);
    if (existing) {
        return;
    }
    
    if (mirror_count >= MAX_MIRRORS) {
        return;
    }
    
    mirrors[mirror_count].x = x;
    mirrors[mirror_count].y = y;
    mirrors[mirror_count].type = type;
    mirrors[mirror_count].rotation = 0;
    mirrors[mirror_count].is_placed = 1;
    mirrors[mirror_count].is_dragging = 0;
    mirror_count++;
    
    moves_made++;
}

void mirror_remove_mirror(int x, int y) {
    for (int i = 0; i < mirror_count; i++) {
        if (mirrors[i].is_placed && mirrors[i].x == x && mirrors[i].y == y) {
            for (int j = i; j < mirror_count - 1; j++) {
                mirrors[j] = mirrors[j + 1];
            }
            mirror_count--;
            break;
        }
    }
}

int mirror_check_win(void) {
    return beam_path.hits_target;
}

void mirror_generate_random_walls(int density) {
    int wall_count = (MAZE_WIDTH - 4) * (MAZE_HEIGHT - 4) * density / 100;
    
    for (int i = 0; i < wall_count; i++) {
        int attempts = 0;
        while (attempts < 50) {
            int x = 2 + (proc_rand() % (MAZE_WIDTH - 4));
            int y = 2 + (proc_rand() % (MAZE_HEIGHT - 4));
            
            if (current_level.grid[y][x] == MIRROR_NONE) {
                int neighbors = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (nx >= 0 && nx < MAZE_WIDTH && ny >= 0 && ny < MAZE_HEIGHT) {
                            if (current_level.grid[ny][nx] == WALL) {
                                neighbors++;
                            }
                        }
                    }
                }
                
                if (neighbors <= 3) {
                    current_level.grid[y][x] = WALL;
                    break;
                }
            }
            attempts++;
        }
    }
}

void mirror_generate_procedural_level(int difficulty) {
    current_level.is_procedural = 1;
    
    char name_prefix[] = "Procedural Level ";
    int i = 0;
    while (name_prefix[i]) {
        current_level.name[i] = name_prefix[i];
        i++;
    }
    current_level.name[i++] = '0' + (current_level_num + 1);
    current_level.name[i] = '\0';
    
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            current_level.grid[y][x] = MIRROR_NONE;
        }
    }
    
    for (int x = 0; x < MAZE_WIDTH; x++) {
        current_level.grid[0][x] = WALL;
        current_level.grid[MAZE_HEIGHT-1][x] = WALL;
    }
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        current_level.grid[y][0] = WALL;
        current_level.grid[y][MAZE_WIDTH-1] = WALL;
    }
    
    int side = proc_rand() % 4;
    MG_Direction directions[] = {MG_DIR_RIGHT, MG_DIR_DOWN, MG_DIR_LEFT, MG_DIR_UP};
    
    switch (side) {
        case 0:
            current_level.laser_x = 2;
            current_level.laser_y = 2 + (proc_rand() % (MAZE_HEIGHT - 4));
            current_level.laser_dir = MG_DIR_RIGHT;
            break;
        case 1:
            current_level.laser_x = 2 + (proc_rand() % (MAZE_WIDTH - 4));
            current_level.laser_y = 2;
            current_level.laser_dir = MG_DIR_DOWN;
            break;
        case 2:
            current_level.laser_x = MAZE_WIDTH - 3;
            current_level.laser_y = 2 + (proc_rand() % (MAZE_HEIGHT - 4));
            current_level.laser_dir = MG_DIR_LEFT;
            break;
        case 3:
            current_level.laser_x = 2 + (proc_rand() % (MAZE_WIDTH - 4));
            current_level.laser_y = MAZE_HEIGHT - 3;
            current_level.laser_dir = MG_DIR_UP;
            break;
    }
    
    int attempts = 0;
    while (attempts < 100) {
        current_level.target_x = 2 + (proc_rand() % (MAZE_WIDTH - 4));
        current_level.target_y = 2 + (proc_rand() % (MAZE_HEIGHT - 4));
        
        int dx = current_level.target_x - current_level.laser_x;
        int dy = current_level.target_y - current_level.laser_y;
        int distance = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
        
        if (distance > 15 && 
            (current_level.target_x != current_level.laser_x || 
             current_level.target_y != current_level.laser_y)) {
            break;
        }
        attempts++;
    }
    
    int wall_density = 10 + (difficulty * 5);
    if (wall_density > 30) wall_density = 30;
    
    mirror_generate_random_walls(wall_density);
    
    for (int y = 2; y < MAZE_HEIGHT - 2; y++) {
        for (int x = 2; x < MAZE_WIDTH - 2; x++) {
            if (current_level.grid[y][x] == WALL) {
                int isolated = 1;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        if (current_level.grid[y+dy][x+dx] == WALL) {
                            isolated = 0;
                            break;
                        }
                    }
                    if (!isolated) break;
                }
                if (isolated && (proc_rand() % 100) < 70) {
                    current_level.grid[y][x] = MIRROR_NONE;
                }
            }
        }
    }
    
    current_level.max_mirrors = 3 + difficulty;
    if (current_level.max_mirrors > 8) current_level.max_mirrors = 8;
    
    int total_mirrors = current_level.max_mirrors;
    int slash_count = total_mirrors / 2;
    int backslash_count = total_mirrors - slash_count;
    
    current_level.available_mirrors[0] = slash_count;
    current_level.available_mirrors[1] = backslash_count;
    current_level.available_mirrors[2] = (difficulty > 2) ? 1 : 0;
    current_level.available_mirrors[3] = (difficulty > 3) ? 1 : 0;
    current_level.available_mirrors[4] = 0;
    
    current_level.grid[current_level.laser_y][current_level.laser_x] = LASER_SOURCE;
    current_level.grid[current_level.target_y][current_level.target_x] = TARGET;
}

void mirror_draw_beam(void) {
    if (beam_path.count == 0) return;
    
    uint8_t colors[] = {VGA_LRED, VGA_YELLOW, VGA_LGREEN, VGA_LCYAN, VGA_LMAGENTA};
    int color_idx = (animation_frame / 2) % 5;
    
    for (int i = 0; i < beam_path.count; i++) {
        int screen_x = MAZE_OFFSET_X + beam_path.points[i].x;
        int screen_y = MAZE_OFFSET_Y + beam_path.points[i].y;
        
        char beam_char;
        switch (beam_path.points[i].dir) {
            case MG_DIR_RIGHT:
            case MG_DIR_LEFT:
                beam_char = 0xC4;
                break;
            case MG_DIR_UP:
            case MG_DIR_DOWN:
                beam_char = 0xB3;
                break;
            default:
                beam_char = 0xFE;
                break;
        }
        
        if (show_beam_animation) {
            int pulse = (i + animation_frame) % 3;
            vga_set_color(colors[(color_idx + pulse) % 5], VGA_BLCK);
        } else {
            vga_set_color(VGA_LRED, VGA_BLCK);
        }
        
        if (current_level.grid[beam_path.points[i].y][beam_path.points[i].x] == MIRROR_NONE) {
            vga_putchr_at(screen_x, screen_y, beam_char);
        }
    }
    
    if (beam_path.hits_target) {
        int target_x = MAZE_OFFSET_X + current_level.target_x;
        int target_y = MAZE_OFFSET_Y + current_level.target_y;
        
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        char sparkles[] = {0x0F, '*', 0x0F, '+', 0x0F};
        vga_putchr_at(target_x, target_y, sparkles[animation_frame % 5]);
    }
}

void mirror_draw_grid(void) {
    vga_begin_batch();
    
    for (int y = 0; y < MAZE_HEIGHT; y++) {
        for (int x = 0; x < MAZE_WIDTH; x++) {
            int screen_x = MAZE_OFFSET_X + x;
            int screen_y = MAZE_OFFSET_Y + y;
            
            CellType cell = current_level.grid[y][x];
            
            if (cell == WALL) {
                vga_set_color(VGA_LGREY, VGA_DGREY);
                vga_putchr_at(screen_x, screen_y, 0xDB);
            } else if (cell == LASER_SOURCE) {
                vga_set_color(VGA_LRED, VGA_BLCK);
                char laser_chars[] = {0x10, 0x11, 0x1B, 0x1A};
                vga_putchr_at(screen_x, screen_y, laser_chars[current_level.laser_dir - 1]);
            } else if (cell == TARGET) {
                uint8_t target_color = beam_path.hits_target ? VGA_LGREEN : VGA_RED;
                vga_set_color(target_color, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 0x04);
            } else {
                vga_set_color(VGA_DGREY, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, 0xFA);
            }
        }
    }
    
    for (int i = 0; i < mirror_count; i++) {
        if (mirrors[i].is_placed && !mirrors[i].is_dragging) {
            int screen_x = MAZE_OFFSET_X + mirrors[i].x;
            int screen_y = MAZE_OFFSET_Y + mirrors[i].y;
            
            char mirror_char;
            uint8_t mirror_color = VGA_LCYAN;
            
            switch (mirrors[i].type) {
                case MIRROR_FORWARD_SLASH:
                    mirror_char = '/';
                    break;
                case MIRROR_BACKSLASH:
                    mirror_char = '\\';
                    break;
                case MIRROR_HORIZONTAL:
                    mirror_char = '-';
                    break;
                case MIRROR_VERTICAL:
                    mirror_char = '|';
                    break;
                default:
                    mirror_char = '?';
                    break;
            }
            
            vga_set_color(mirror_color, VGA_BLCK);
            vga_putchr_at(screen_x, screen_y, mirror_char);
        }
    }
    
    mirror_draw_beam();
    
    if (dragging_mirror) {
        int mouse_x, mouse_y;
        mouse_get_position(&mouse_x, &mouse_y);
        
        char mirror_char;
        switch (dragging_mirror->type) {
            case MIRROR_FORWARD_SLASH: mirror_char = '/'; break;
            case MIRROR_BACKSLASH: mirror_char = '\\'; break;
            case MIRROR_HORIZONTAL: mirror_char = '-'; break;
            case MIRROR_VERTICAL: mirror_char = '|'; break;
            default: mirror_char = '?'; break;
        }
        
        vga_set_color(VGA_YELLOW, VGA_DGREY);
        vga_putchr_at(mouse_x, mouse_y, mirror_char);
    }
    
    vga_end_batch();
}

void mirror_draw_mirror_palette(void) {
    int palette_x = 40;
    int palette_y = 3;
    
    vga_draw_box_double(palette_x, palette_y, 38, 18, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* title = "MIRROR PALETTE";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(palette_x + 12 + i, palette_y, title[i]);
    }
    
    const char* mirror_names[] = {
        "Forward Slash  /",
        "Backslash      \\",
        "Horizontal     -",
        "Vertical       |"
    };
    
    CellType mirror_types[] = {
        MIRROR_FORWARD_SLASH,
        MIRROR_BACKSLASH,
        MIRROR_HORIZONTAL,
        MIRROR_VERTICAL
    };
    
    int y_pos = palette_y + 2;
    for (int i = 0; i < 4; i++) {
        int available = current_level.available_mirrors[i];
        int used = 0;
        
        for (int j = 0; j < mirror_count; j++) {
            if (mirrors[j].is_placed && mirrors[j].type == mirror_types[i]) {
                used++;
            }
        }
        
        uint8_t color = (available - used > 0) ? VGA_WHITE : VGA_DGREY;
        vga_set_color(color, VGA_BLCK);
        
        int x = palette_x + 2;
        for (int j = 0; mirror_names[i][j]; j++) {
            vga_putchr_at(x++, y_pos, mirror_names[i][j]);
        }
        
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        char count_str[10];
        int pos = 0;
        count_str[pos++] = ' ';
        count_str[pos++] = '(';
        count_str[pos++] = '0' + (available - used);
        count_str[pos++] = '/';
        count_str[pos++] = '0' + available;
        count_str[pos++] = ')';
        count_str[pos] = '\0';
        
        for (int j = 0; count_str[j]; j++) {
            vga_putchr_at(x++, y_pos, count_str[j]);
        }
        
        y_pos += 3;
    }
    
    vga_draw_separator(palette_x + 1, palette_y + 15, 36, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* hint1 = "Click mirror, then click";
    const char* hint2 = "grid to place it!";
    const char* hint3 = "Right-click to remove";
    
    y_pos = palette_y + 16;
    int x = palette_x + 2;
    for (int i = 0; hint1[i]; i++) {
        vga_putchr_at(x++, y_pos, hint1[i]);
    }
    
    y_pos++;
    x = palette_x + 7;
    for (int i = 0; hint2[i]; i++) {
        vga_putchr_at(x++, y_pos, hint2[i]);
    }
    
    y_pos++;
    x = palette_x + 4;
    for (int i = 0; hint3[i]; i++) {
        vga_putchr_at(x++, y_pos, hint3[i]);
    }
}

void mirror_draw_ui(void) {
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== MIRROR MAZE ===", 0);
    
    vga_set_color(VGA_WHITE, VGA_BLUE);
    
    int x = 2;
    for (int i = 0; current_level.name[i]; i++) {
        vga_putchr_at(x++, 1, current_level.name[i]);
    }
    
    vga_draw_box_single(0, 22, 40, 3, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    char info[40];
    int pos = 0;
    const char* moves_label = "Moves: ";
    for (int i = 0; moves_label[i]; i++) info[pos++] = moves_label[i];
    
    int temp = moves_made;
    if (temp == 0) {
        info[pos++] = '0';
    } else {
        char digits[10];
        int dc = 0;
        while (temp > 0) {
            digits[dc++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = dc - 1; i >= 0; i--) {
            info[pos++] = digits[i];
        }
    }
    
    info[pos++] = ' ';
    info[pos++] = '|';
    info[pos++] = ' ';
    
    const char* level_label = "Level: ";
    for (int i = 0; level_label[i]; i++) info[pos++] = level_label[i];
    info[pos++] = '0' + (current_level_num + 1);
    info[pos++] = '/';
    info[pos++] = '3';
    
    info[pos] = '\0';
    
    for (int i = 0; i < pos; i++) {
        vga_putchr_at(2 + i, 23, info[i]);
    }
    
    vga_draw_box_single(40, 22, 40, 3, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    const char* status = beam_path.hits_target ? "TARGET HIT!" : "Aim the laser...";
    uint8_t status_color = beam_path.hits_target ? VGA_LGREEN : VGA_LGREY;
    
    vga_set_color(status_color, VGA_BLCK);
    x = 42;
    for (int i = 0; status[i]; i++) {
        vga_putchr_at(x++, 23, status[i]);
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* controls = "R:Reset ESC:Menu";
    x = 42;
    for (int i = 0; controls[i]; i++) {
        vga_putchr_at(x++, 24, controls[i]);
    }
}

void mirror_draw_menu(void) {
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    const char* title[] = {
        "  __  __ _                        ",
        " |  \\/  (_)                       ",
        " | \\  / |_ _ __ _ __ ___  _ __   ",
        " | |\\/| | | '__| '__/ _ \\| '__|  ",
        " | |  | | | |  | | | (_) | |     ",
        " |_|  |_|_|_|  |_|  \\___/|_|     ",
        "  __  __                          ",
        " |  \\/  |                         ",
        " | \\  / | __ _ ______  ___  ",
        " | |\\/| |/ _` |_  / _ \\       ",
        " | |  | | (_| |/ /  __/           ",
        " |_|  |_|\\__,_/___\\___|           "
    };
    
    for (int i = 0; i < 12; i++) {
        int len = 0;
        while (title[i][len]) len++;
        int x = (80 - len) / 2;
        
        uint8_t colors[] = {VGA_LCYAN, VGA_CYAN, VGA_LGREEN, VGA_GREEN, 
                           VGA_YELLOW, VGA_LRED, VGA_RED, VGA_MAGENTA,
                           VGA_LMAGENTA, VGA_LCYAN, VGA_CYAN, VGA_LGREEN};
        vga_set_color(colors[i], VGA_BLCK);
        
        for (int j = 0; title[i][j]; j++) {
            vga_putchr_at(x + j, 2 + i, title[i][j]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* subtitle = "Reflect the laser to hit the target!";
    int len = 0;
    while (subtitle[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((80 - len) / 2 + i, 15, subtitle[i]);
    }
    
    const char* options[] = {
        "Level 1: Tutorial",
        "Level 2: Corner Shot",
        "Level 3: Maze Runner",
        "Procedural Level (Easy)",
        "Procedural Level (Medium)",
        "Procedural Level (Hard)",
        "Help",
        "Exit"
    };
    
    for (int i = 0; i < 8; i++) {
        int selected = (i == menu_selection);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(18, 17 + i, 44, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 20;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, 17 + i, prefix[j]);
        }
        
        for (int j = 0; options[i][j]; j++) {
            vga_putchr_at(x++, 17 + i, options[i][j]);
        }
    }
}

void mirror_draw_help(void) {
    vga_clear();
    
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_YELLOW, VGA_BLUE);
    vga_print_centered("=== HELP & CONTROLS ===", 0);
    
    const char* help_text[] = {
        "OBJECTIVE:",
        "  Guide the laser beam from source to target using mirrors!",
        "",
        "HOW TO PLAY:",
        "  1. Click on a mirror in the palette (right side)",
        "  2. Click on the grid to place it",
        "  3. Right-click on a mirror to remove it",
        "  4. Watch the beam path update in real-time!",
        "",
        "MIRROR TYPES:",
        "  /  Forward Slash  - Reflects beam 90 degrees",
        "  \\  Backslash      - Reflects beam 90 degrees",
        "  -  Horizontal     - Bounces vertical beams",
        "  |  Vertical       - Bounces horizontal beams",
        "",
        "KEYBOARD CONTROLS:",
        "  R: Reset level",
        "  ESC: Return to menu",
        "  SPACE: Pause/resume beam animation",
        "",
        "MOUSE CONTROLS:",
        "  Left Click: Select mirror from palette / Place mirror",
        "  Right Click: Remove mirror from grid",
        "",
        "TIPS:",
        "  - Each level has a limited number of mirrors",
        "  - The beam stops at walls",
        "  - Try to use the minimum number of moves!",
        "",
        "Press any key to continue..."
    };
    
    int y = 2;
    for (int i = 0; i < 22; i++) {
        int len = 0;
        while (help_text[i][len]) len++;
        
        uint8_t color = VGA_WHITE;
        if (help_text[i][0] != ' ' && help_text[i][0] != '\0') {
            color = VGA_YELLOW;
        }
        
        vga_set_color(color, VGA_BLCK);
        for (int j = 0; j < len; j++) {
            vga_putchr_at(3 + j, y, help_text[i][j]);
        }
        y++;
    }
    
    wait_for_char();
}

void mirror_draw_level_complete(void) {
    vga_draw_box_double(15, 8, 50, 9, VGA_LGREEN, VGA_BLCK);
    vga_fill_rect(16, 9, 48, 7, ' ', VGA_YELLOW, VGA_DGREY);
    
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    const char* win_text = "LEVEL COMPLETE!";
    int len = 0;
    while (win_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 10, win_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_DGREY);
    char info[40];
    int pos = 0;
    const char* moves_label = "Moves used: ";
    for (int i = 0; moves_label[i]; i++) info[pos++] = moves_label[i];
    
    int temp = moves_made;
    if (temp == 0) {
        info[pos++] = '0';
    } else {
        char digits[10];
        int dc = 0;
        while (temp > 0) {
            digits[dc++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = dc - 1; i >= 0; i--) {
            info[pos++] = digits[i];
        }
    }
    info[pos] = '\0';
    
    len = 0;
    while (info[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 12, info[i]);
    }
    
    if (moves_made < best_moves[current_level_num]) {
        best_moves[current_level_num] = moves_made;
        vga_set_color(VGA_LGREEN, VGA_DGREY);
        const char* new_best = "NEW BEST!";
        len = 0;
        while (new_best[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at(40 - len / 2 + i, 13, new_best[i]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_DGREY);
    const char* inst1 = current_level.is_procedural ? 
        "ENTER: New random level  R: Retry" : "Press ENTER for next level";
    const char* inst2 = "ESC: Menu";
    
    len = 0;
    while (inst1[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 14, inst1[i]);
    }
    
    len = 0;
    while (inst2[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 15, inst2[i]);
    }
}

void mirror_handle_mouse(void) {
    MouseEvent event;
    
    while (mouse_has_event()) {
        mouse_get_event(&event);
        
        int mouse_x, mouse_y;
        mouse_get_position(&mouse_x, &mouse_y);
        
        if (event.type == MOUSE_EVENT_BUTTON_PRESS) {
            if (event.buttons & MOUSE_LEFT_BUTTON) {
                int palette_x = 40;
                int palette_y = 3;
                
                if (mouse_x >= palette_x + 2 && mouse_x < palette_x + 36) {
                    int relative_y = mouse_y - (palette_y + 2);
                    
                    if (relative_y >= 0 && relative_y < 12) {
                        int item = relative_y / 3;
                        if (item >= 0 && item < 4) {
                            CellType mirror_types[] = {
                                MIRROR_FORWARD_SLASH,
                                MIRROR_BACKSLASH,
                                MIRROR_HORIZONTAL,
                                MIRROR_VERTICAL
                            };
                            
                            int available = current_level.available_mirrors[item];
                            int used = 0;
                            
                            for (int j = 0; j < mirror_count; j++) {
                                if (mirrors[j].is_placed && mirrors[j].type == mirror_types[item]) {
                                    used++;
                                }
                            }
                            
                            if (available - used > 0) {
                                selected_palette_item = item;
                                
                                if (mirror_count < MAX_MIRRORS) {
                                    mirrors[mirror_count].type = mirror_types[item];
                                    mirrors[mirror_count].is_placed = 0;
                                    mirrors[mirror_count].is_dragging = 1;
                                    dragging_mirror = &mirrors[mirror_count];
                                    mirror_count++;
                                }
                            }
                        }
                    }
                } else if (dragging_mirror) {
                    int grid_x = mouse_x - MAZE_OFFSET_X;
                    int grid_y = mouse_y - MAZE_OFFSET_Y;
                    
                    if (grid_x >= 1 && grid_x < MAZE_WIDTH - 1 && 
                        grid_y >= 1 && grid_y < MAZE_HEIGHT - 1) {
                        
                        if (current_level.grid[grid_y][grid_x] == MIRROR_NONE) {
                            Mirror* existing = mirror_get_at(grid_x, grid_y);
                            if (!existing) {
                                dragging_mirror->x = grid_x;
                                dragging_mirror->y = grid_y;
                                dragging_mirror->is_placed = 1;
                                dragging_mirror->is_dragging = 0;
                                dragging_mirror = 0;
                                
                                moves_made++;
                                mirror_calculate_beam();
                            }
                        }
                    } else {
                        mirror_count--;
                        dragging_mirror = 0;
                    }
                }
            } else if (event.buttons & MOUSE_RIGHT_BUTTON) {
                int grid_x = mouse_x - MAZE_OFFSET_X;
                int grid_y = mouse_y - MAZE_OFFSET_Y;
                
                if (grid_x >= 0 && grid_x < MAZE_WIDTH && 
                    grid_y >= 0 && grid_y < MAZE_HEIGHT) {
                    mirror_remove_mirror(grid_x, grid_y);
                    mirror_calculate_beam();
                }
                
                if (dragging_mirror) {
                    mirror_count--;
                    dragging_mirror = 0;
                }
            }
        }
    }
}

void mirror_handle_input(void) {
    keyboard_poll();
    
    if (!has_key()) return;
    
    char c = get_char();
    
    if (game_state == MIRROR_MENU) {
        if (c == 0x11) {
            menu_selection = (menu_selection - 1 + 8) % 8;
        } else if (c == 0x12) {
            menu_selection = (menu_selection + 1) % 8;
        } else if (c == '\n') {
            if (menu_selection >= 0 && menu_selection <= 5) {
                mirror_load_level(menu_selection);
                game_state = MIRROR_PLAYING;
            } else if (menu_selection == 6) {
                mirror_draw_help();
            } else if (menu_selection == 7) {
                return;
            }
        } else if (c == 27) {
            return;
        }
    } else if (game_state == MIRROR_PLAYING) {
        if (c == 'r' || c == 'R') {
            mirror_reset_level();
        } else if (c == ' ') {
            show_beam_animation = !show_beam_animation;
        } else if (c == 27) {
            game_state = MIRROR_MENU;
        }
        
        if (mirror_check_win()) {
            game_state = MIRROR_LEVEL_COMPLETE;
        }
    } else if (game_state == MIRROR_LEVEL_COMPLETE) {
        if (c == '\n') {
            if (current_level_num < 5) {
                mirror_load_level(current_level_num + 1);
                game_state = MIRROR_PLAYING;
            } else if (current_level.is_procedural) {
                proc_rand_seed = pit_get_ticks() * (current_level_num + 1);
                mirror_load_level(current_level_num);
                game_state = MIRROR_PLAYING;
            } else {
                game_state = MIRROR_MENU;
            }
        } else if (c == 'r' || c == 'R') {
            if (current_level.is_procedural) {
                proc_rand_seed = pit_get_ticks() * (current_level_num + 1);
            }
            mirror_load_level(current_level_num);
            game_state = MIRROR_PLAYING;
        } else if (c == 27) {
            game_state = MIRROR_MENU;
        }
    }
}

void mirror_game_run(void) {
    mirror_init();
    
    while (1) {
        animation_frame++;
        
        mirror_handle_input();
        mirror_handle_mouse();
        
        if (game_state == MIRROR_MENU) {
            mirror_draw_menu();
            pit_delay_ms(50);
            
            if (menu_selection == 7) {
                keyboard_poll();
                if (has_key() && get_char() == '\n') {
                    break;
                }
            }
        } else if (game_state == MIRROR_PLAYING) {
            vga_clear();
            mirror_draw_ui();
            mirror_draw_grid();
            mirror_draw_mirror_palette();
            
            pit_delay_ms(50);
        } else if (game_state == MIRROR_LEVEL_COMPLETE) {
            mirror_draw_level_complete();
            pit_delay_ms(50);
        }
    }
}

void mirror_game_cleanup(void) {
    mouse_hide_cursor();
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}