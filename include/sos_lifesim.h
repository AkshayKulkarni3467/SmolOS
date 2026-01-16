#ifndef INCLUDE_SMOLOS_LIFE_H
#define INCLUDE_SMOLOS_LIFE_H

#include "sos_stdint.h"

#define LIFE_WIDTH 70
#define LIFE_HEIGHT 20
#define LIFE_OFFSET_X 5
#define LIFE_OFFSET_Y 3

typedef enum {
    LIFE_MENU,
    LIFE_RUNNING,
    LIFE_PAUSED,
    LIFE_HELP
} LifeState;

typedef enum {
    DRAW_MODE_SINGLE,
    DRAW_MODE_SPRAY,
    DRAW_MODE_LINE,
    DRAW_MODE_RECT,
    DRAW_MODE_GLIDER,
    DRAW_MODE_LWSS,
    DRAW_MODE_PULSAR,
    DRAW_MODE_GOSPER_GLIDER_GUN,
    DRAW_MODE_ERASE
} DrawMode;

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    const char* name;
    int width;
    int height;
    const uint8_t* pattern;
} Pattern;

void life_init(void);
void life_reset(void);
void life_update(void);
void life_draw_grid(void);
void life_draw_ui(void);
void life_draw_menu(void);
void life_draw_help(void);
void life_handle_input(void);
void life_handle_mouse(void);
void life_set_cell(int x, int y, int alive);
int life_get_cell(int x, int y);
void life_toggle_cell(int x, int y);
int life_count_neighbors(int x, int y);
void life_clear_grid(void);
void life_randomize(int density);
void life_draw_pattern(int x, int y, const Pattern* pattern);
void life_draw_glider(int x, int y);
void life_draw_lwss(int x, int y);
void life_draw_pulsar(int x, int y);
void life_draw_gosper_gun(int x, int y);
void life_spray_paint(int x, int y);
void life_game_run(void);
void life_game_cleanup(void);

#endif // INCLUDE_SMOLOS_LIFE_H