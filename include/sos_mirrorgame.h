#ifndef INCLUDE_SMOLOS_MIRROR_H
#define INCLUDE_SMOLOS_MIRROR_H

#include "sos_stdint.h"

#define MAZE_WIDTH 35
#define MAZE_HEIGHT 18
#define MAZE_OFFSET_X 2
#define MAZE_OFFSET_Y 3

#define MAX_MIRRORS 20
#define MAX_BEAM_POINTS 200
#define MAX_LEVELS 10

typedef enum {
    MIRROR_MENU,
    MIRROR_PLAYING,
    MIRROR_LEVEL_COMPLETE,
    MIRROR_PAUSED,
    MIRROR_HELP
} MirrorState;

typedef enum {
    MG_DIR_NONE,
    MG_DIR_RIGHT,
    MG_DIR_DOWN,
    MG_DIR_LEFT,
    MG_DIR_UP
} MG_Direction;

typedef enum {
    MIRROR_NONE = 0,
    MIRROR_FORWARD_SLASH,   
    MIRROR_BACKSLASH,      
    MIRROR_HORIZONTAL,      
    MIRROR_VERTICAL,        
    WALL,
    LASER_SOURCE,
    TARGET,
    BEAM_SPLITTER
} CellType;

typedef struct {
    int x;
    int y;
    CellType type;
    int rotation;
    int is_placed;
    int is_dragging;
} Mirror;

typedef struct {
    int x;
    int y;
    MG_Direction dir;
} BeamPoint;

typedef struct {
    int laser_x;
    int laser_y;
    MG_Direction laser_dir;
    int target_x;
    int target_y;
    int max_mirrors;
    int available_mirrors[5];
    char name[40];
    CellType grid[MAZE_HEIGHT][MAZE_WIDTH];
    int is_procedural;
} Level;

typedef struct {
    BeamPoint points[MAX_BEAM_POINTS];
    int count;
    int hits_target;
} BeamPath;

void mirror_init(void);
void mirror_reset_level(void);
void mirror_load_level(int level_num);
void mirror_draw_grid(void);
void mirror_draw_ui(void);
void mirror_draw_menu(void);
void mirror_draw_help(void);
void mirror_draw_level_complete(void);
void mirror_handle_input(void);
void mirror_handle_mouse(void);
void mirror_calculate_beam(void);
MG_Direction mirror_reflect(MG_Direction incoming, CellType mirror_type);
void mirror_draw_beam(void);
void mirror_draw_particle_effect(int x, int y);
int mirror_check_win(void);
void mirror_place_mirror(int x, int y, CellType type);
void mirror_remove_mirror(int x, int y);
Mirror* mirror_get_at(int x, int y);
void mirror_draw_mirror_palette(void);
void mirror_generate_procedural_level(int difficulty);
void mirror_generate_random_walls(int density);
int mirror_test_solvability(void);
void mirror_game_run(void);
void mirror_game_cleanup(void);

#endif // INCLUDE_SMOLOS_MIRROR_H