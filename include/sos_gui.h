#ifndef INCLUDE_SMOLOS_GUI_H
#define INCLUDE_SMOLOS_GUI_H

#include "sos_stdint.h"


typedef enum {
    WINDOW_NORMAL,
    WINDOW_MINIMIZED,
    WINDOW_MAXIMIZED,
    WINDOW_CLOSED
} WindowState;


typedef struct {
    int id;
    int x, y;
    int width, height;
    char title[40];
    WindowState state;
    uint8_t color;
    int is_focused;
    int has_close_button;
    void (*content_render)(int x, int y, int w, int h);
    void (*on_click)(int mx, int my);
} Window;


typedef struct {
    int x, y;
    int width, height;
    char label[20];
    uint8_t color;
    int is_hovered;
    void (*on_click)(void);
} Button;


typedef struct {
    int x, y;
    char label[16];
    char icon;
    uint8_t color;
    void (*on_click)(void);
} DesktopIcon;


void gui_init(void);
void gui_run(void);
void gui_shutdown(void);


int gui_create_window(const char* title, int x, int y, int w, int h, uint8_t color);
void gui_close_window(int id);
void gui_focus_window(int id);
void gui_draw_window(Window* win);


void gui_draw_desktop(void);
void gui_draw_taskbar(void);
void gui_add_desktop_icon(const char* label, char icon, int x, int y, void (*callback)(void));


void gui_app_terminal(void);
void gui_app_calculator(void);
void gui_app_about(void);
void gui_app_file_browser(void);
void gui_app_clock(void);
void gui_app_art_gallery(void);

void gui_app_games_launcher(void);
void gui_app_network_tools(void);

#endif // INCLUDE_SMOLOS_GUI_H