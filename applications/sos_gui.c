#include "sos_gui.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_mouse.h"
#include "sos_keyboard.h"
#include "sos_rtc.h"
#include "sos_pit.h"
#include "sos_memory.h"
#include "sos_string.h"
#include "sos_artgallery.h"
#include "sos_calculator.h"
#include "sos_filemanager.h"
#include "sos_todolist.h"
#include "sos_shell.h"
#include "sos_cmds.h"


static Window windows[8];
static int window_count = 0;
static int focused_window = -1;
static int running = 1;

static DesktopIcon icons[20];  
static int icon_count = 0;

static int dragging_window = -1;
static int drag_offset_x = 0;
static int drag_offset_y = 0;


static int anim_frame = 0;


#define GUI_DESKTOP_BG    VGA_LBLUE
#define GUI_TASKBAR_BG    VGA_DGREY
#define GUI_WINDOW_TITLE  VGA_BLUE
#define GUI_WINDOW_BG     VGA_LGREY


static int point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void print_with_scroll(const char* text) {
    vga_println((char*)text);
    shell_check_scroll();
}


int gui_create_window(const char* title, int x, int y, int w, int h, uint8_t color) {
    if (window_count >= 8) return -1;
    
    Window* win = &windows[window_count];
    win->id = window_count;
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    strcpy(win->title, (char*)title);
    win->state = WINDOW_NORMAL;
    win->color = color;
    win->is_focused = 1;
    win->has_close_button = 1;
    win->content_render = 0;
    win->on_click = 0;
    
    focused_window = window_count;
    

    for (int i = 0; i < window_count; i++) {
        windows[i].is_focused = 0;
    }
    
    return window_count++;
}

void gui_close_window(int id) {
    if (id < 0 || id >= window_count) return;
    windows[id].state = WINDOW_CLOSED;
}

void gui_focus_window(int id) {
    if (id < 0 || id >= window_count) return;
    
    for (int i = 0; i < window_count; i++) {
        windows[i].is_focused = 0;
    }
    
    windows[id].is_focused = 1;
    focused_window = id;
}

void gui_draw_window(Window* win) {
    if (win->state == WINDOW_CLOSED) return;
    if (win->state == WINDOW_MINIMIZED) return;
    

    vga_fill_rect(win->x, win->y, win->width, win->height, ' ', VGA_WHITE, GUI_WINDOW_BG);
    

    uint8_t title_color = win->is_focused ? GUI_WINDOW_TITLE : VGA_DGREY;
    vga_fill_rect(win->x, win->y, win->width, 1, ' ', VGA_WHITE, title_color);
    

    vga_set_color(VGA_WHITE, title_color);
    vga_t_column = win->x + 2;
    vga_t_row = win->y;
    vga_setcursor(win->x + 2, win->y);
    
    int title_len = 0;
    while (win->title[title_len] && title_len < win->width - 5) {
        vga_putchr_at(win->x + 2 + title_len, win->y, win->title[title_len]);
        title_len++;
    }
    

    if (win->has_close_button) {
        vga_set_color(VGA_LRED, title_color);
        vga_putchr_at(win->x + win->width - 2, win->y, 'X');
    }
    

    vga_draw_box_single(win->x, win->y, win->width, win->height, VGA_DGREY, GUI_WINDOW_BG);
    

    if (win->content_render) {
        win->content_render(win->x + 1, win->y + 2, win->width - 2, win->height - 3);
    }
}


void gui_draw_desktop(void) {

    for (int y = 0; y < 23; y++) {
        for (int x = 0; x < 80; x++) {
            char c = ((x + y) % 4 == 0) ? 0xB0 : ' ';
            vga_putchr_at(x, y, c);
            vga_set_color(GUI_DESKTOP_BG, GUI_DESKTOP_BG);
        }
    }
    
    const char* logo[] = {
        " _____            _ _____ _____ ",
        "|   __|_____ ___ | |     |   __|",
        "|__   |     | . || |  |  |__   |",
        "|_____|_|_|_|___||_|_____|_____|"
    };
    
    vga_set_color(VGA_WHITE, GUI_DESKTOP_BG);
    for (int i = 0; i < 4; i++) {
        int x = 2;
        for (int j = 0; logo[i][j]; j++) {
            if (x < 78 && i + 2 < 23) {
                vga_putchr_at(x, i + 2, logo[i][j]);
            }
            x++;
        }
    }
    

    for (int i = 0; i < icon_count; i++) {

        vga_set_color(VGA_WHITE, GUI_DESKTOP_BG);
        vga_putchr_at(icons[i].x, icons[i].y, icons[i].icon);
        

        vga_set_color(VGA_YELLOW, GUI_DESKTOP_BG);
        int label_len = strlen(icons[i].label);
        int label_x = icons[i].x - label_len / 2;
        for (int j = 0; j < label_len; j++) {
            int pos_x = label_x + j;
            if (pos_x >= 0 && pos_x < 80 && icons[i].y + 1 < 23) {
                vga_putchr_at(pos_x, icons[i].y + 1, icons[i].label[j]);
            }
        }
        

        vga_set_color(VGA_DGREY, GUI_DESKTOP_BG);
        if (icons[i].x + 1 < 80 && icons[i].y + 1 < 23) {
            vga_putchr_at(icons[i].x + 1, icons[i].y + 1, 0xB0);
        }
    }
}

void gui_draw_taskbar(void) {

    vga_fill_rect(0, 23, 80, 2, ' ', VGA_WHITE, GUI_TASKBAR_BG);
    

    vga_set_color(VGA_LGREEN, GUI_TASKBAR_BG);
    vga_putchr_at(1, 23, '[');
    vga_set_color(VGA_YELLOW, GUI_TASKBAR_BG);
    vga_putchr_at(2, 23, 'S');
    vga_set_color(VGA_LGREEN, GUI_TASKBAR_BG);
    vga_putchr_at(3, 23, ']');
    

    int x = 6;
    for (int i = 0; i < window_count; i++) {
        if (windows[i].state != WINDOW_CLOSED && x < 55) {
            uint8_t color = windows[i].is_focused ? VGA_LCYAN : VGA_DGREY;
            vga_set_color(VGA_BLCK, color);
            
            vga_putchr_at(x, 23, '[');
            x++;
            
            int len = 0;
            while (windows[i].title[len] && len < 10 && x < 54) {
                vga_putchr_at(x, 23, windows[i].title[len]);
                x++;
                len++;
            }
            
            vga_putchr_at(x, 23, ']');
            x += 2;
        }
    }
    

    vga_set_color(VGA_LCYAN, GUI_TASKBAR_BG);
    vga_putchr_at(56, 23, 0x0F);  
    vga_set_color(VGA_LGREEN, GUI_TASKBAR_BG);
    vga_putchr_at(58, 23, 0xFE);  
    vga_set_color(VGA_YELLOW, GUI_TASKBAR_BG);
    vga_putchr_at(60, 23, 0x0E);  
    

    RTCTime time;
    rtc_get_local_time(&time);
    char time_str[16];
    rtc_format_time(&time, time_str);
    
    vga_set_color(VGA_YELLOW, GUI_TASKBAR_BG);
    int time_len = strlen(time_str);
    for (int i = 0; i < time_len; i++) {
        vga_putchr_at(80 - time_len + i, 23, time_str[i]);
    }
    

    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* info = "SmolOS v1.0 | Press ESC to exit GUI";
    int info_len = strlen(info);
    for (int i = 0; i < info_len; i++) {
        vga_putchr_at(1 + i, 24, info[i]);
    }
}

void gui_add_desktop_icon(const char* label, char icon, int x, int y, void (*callback)(void)) {
    if (icon_count >= 20) return;
    
    strcpy(icons[icon_count].label, (char*)label);
    icons[icon_count].icon = icon;
    icons[icon_count].x = x;
    icons[icon_count].y = y;
    icons[icon_count].color = VGA_WHITE;
    icons[icon_count].on_click = callback;
    icon_count++;
}


void gui_app_terminal(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    run_shell();
    running = 1;
}

void gui_app_calculator(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    calc_command();
    running = 1;
}

void gui_app_about(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    for(int i = 0; i < WIDTH; i++) {
        for(int j = 0; j < HEIGHT; j++) {
            vga_putchr_at(i, j, ' ');
        }
    }
    cmd_about((CommandArgs){0});
    wait_for_char();
    running = 1;
}

void gui_app_clock(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    for(int i = 0; i < WIDTH; i++) {
        for(int j = 0; j < HEIGHT; j++) {
            vga_putchr_at(i, j, ' ');
        }
    }
    cmd_clock((CommandArgs){0});
    running = 1;
}

void gui_app_file_browser(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    file_manager_command();
    running = 1;
}

void gui_app_art_gallery(void) {
    running = 0;
    vga_clear();
    art_gallery_main();
    running = 1;
}

void gui_app_todolist(void) {
    running = 0;
    vga_clear();
    cmd_todo();
    running = 1;
}

void gui_app_banner(void) {
    running = 0;
    vga_clear();
    cmd_banner((CommandArgs){0});
    running = 1;
}

void gui_app_reaction(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    for(int i = 0; i < WIDTH; i++) {
        for(int j = 0; j < HEIGHT; j++) {
            vga_putchr_at(i, j, ' ');
        }
    }
    cmd_reaction((CommandArgs){0});
    wait_for_char();
    running = 1;
}

void gui_app_perfmon(void) {
    running = 0;
    vga_clear();
    cmd_perfmon((CommandArgs){0});
    running = 1;
}

void gui_app_stopwatch(void) {
    running = 0;
    vga_clear();
    cmd_stopwatch((CommandArgs){0});
    running = 1;
}

void gui_app_datetime(void) {
    running = 0;
    vga_clear();
    cmd_datetime((CommandArgs){0});
    running = 1;
}

void gui_app_mousedraw(void) {
    running = 0;
    vga_clear();
    cmd_mousedraw((CommandArgs){0});
    running = 1;
}

void gui_app_mousetest(void) {
    running = 0;
    vga_clear();
    cmd_mousetest((CommandArgs){0});
    running = 1;
}

void gui_app_sysinfo(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_sysinfo((CommandArgs){0});
    running = 1;
}

void gui_app_benchmark(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_benchmark((CommandArgs){0});
    wait_for_char();
    running = 1;
}

void gui_app_diskinfo(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_diskinfo_enhanced((CommandArgs){0});
    wait_for_char();
    running = 1;
}

void gui_app_uptime(void) {
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_uptime_rtc((CommandArgs){0});
    wait_for_char();
    running = 1;
}


void gui_init(void) {
    window_count = 0;
    icon_count = 0;
    focused_window = -1;
    running = 1;
    anim_frame = 0;
    

    gui_add_desktop_icon("Terminal", 0x0F, 42, 8, gui_app_terminal);
    gui_add_desktop_icon("Calculator", 0xF7, 52, 8, gui_app_calculator);
    gui_add_desktop_icon("Files", 0xFE, 62, 8, gui_app_file_browser);
    gui_add_desktop_icon("Todo", 0x09, 72, 8, gui_app_todolist);
    

    gui_add_desktop_icon("Art", 0x0C, 42, 11, gui_app_art_gallery);
    gui_add_desktop_icon("Banner", 0x13, 52, 11, gui_app_banner);
    gui_add_desktop_icon("Draw", 0x04, 62, 11, gui_app_mousedraw);
    gui_add_desktop_icon("Reaction", 0x02, 72, 11, gui_app_reaction);
    

    gui_add_desktop_icon("Clock", 0x0E, 42, 14, gui_app_clock);
    gui_add_desktop_icon("Stopwatch", 0x07, 52, 14, gui_app_stopwatch);
    gui_add_desktop_icon("DateTime", 0x0B, 62, 14, gui_app_datetime);
    gui_add_desktop_icon("Uptime", 0x01, 72, 14, gui_app_uptime);
    

    gui_add_desktop_icon("PerfMon", 0x10, 42, 17, gui_app_perfmon);
    gui_add_desktop_icon("SysInfo", 'i', 52, 17, gui_app_sysinfo);
    gui_add_desktop_icon("DiskInfo", 0xDB, 62, 17, gui_app_diskinfo);
    gui_add_desktop_icon("Benchmark", 0x0D, 72, 17, gui_app_benchmark);
    

    gui_add_desktop_icon("MouseTest", 0x1A, 42, 20, gui_app_mousetest);
    gui_add_desktop_icon("About", '?', 52, 20, gui_app_about);
}

void gui_run(void) {
    mouse_show_cursor();
    mouse_set_cursor_char(0x1A);  
    
    int last_mouse_buttons = 0;
    
    while (running) {
        vga_begin_batch();
        

        gui_draw_desktop();
        

        for (int i = 0; i < window_count; i++) {
            if (windows[i].state != WINDOW_CLOSED && !windows[i].is_focused) {
                gui_draw_window(&windows[i]);
            }
        }
        

        if (focused_window >= 0 && windows[focused_window].state != WINDOW_CLOSED) {
            gui_draw_window(&windows[focused_window]);
        }
        

        gui_draw_taskbar();
        
        vga_end_batch();
        

        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            if (c == 27) {  
                running = 0;
            }
        }
        

        MouseEvent event;
        while (mouse_get_event(&event)) {
            if (event.type == MOUSE_EVENT_BUTTON_PRESS && (event.buttons & MOUSE_LEFT_BUTTON)) {

                for (int i = 0; i < icon_count; i++) {
                    if (point_in_rect(event.x, event.y, icons[i].x - 1, icons[i].y, 3, 2)) {
                        if (icons[i].on_click) {
                            icons[i].on_click();
                            if (!running) break;
                        }
                    }
                }
                

                for (int i = window_count - 1; i >= 0; i--) {
                    if (windows[i].state == WINDOW_CLOSED) continue;
                    
                    Window* win = &windows[i];
                    

                    if (win->has_close_button && 
                        point_in_rect(event.x, event.y, win->x + win->width - 2, win->y, 1, 1)) {
                        gui_close_window(win->id);
                        break;
                    }
                    

                    if (point_in_rect(event.x, event.y, win->x, win->y, win->width, 1)) {
                        gui_focus_window(win->id);
                        dragging_window = win->id;
                        drag_offset_x = event.x - win->x;
                        drag_offset_y = event.y - win->y;
                        break;
                    }
                    

                    if (point_in_rect(event.x, event.y, win->x, win->y, win->width, win->height)) {
                        gui_focus_window(win->id);
                        if (win->on_click) {
                            win->on_click(event.x - win->x, event.y - win->y);
                        }
                        break;
                    }
                }
            }
            

            if (event.type == MOUSE_EVENT_MOVE && dragging_window >= 0) {
                if (mouse_is_left_pressed()) {
                    Window* win = &windows[dragging_window];
                    win->x = event.x - drag_offset_x;
                    win->y = event.y - drag_offset_y;
                    

                    if (win->x < 0) win->x = 0;
                    if (win->y < 0) win->y = 0;
                    if (win->x + win->width > 80) win->x = 80 - win->width;
                    if (win->y + win->height > 23) win->y = 23 - win->height;
                } else {
                    dragging_window = -1;
                }
            }
            
            if (event.type == MOUSE_EVENT_BUTTON_RELEASE) {
                dragging_window = -1;
            }
        }
        
        anim_frame++;
        pit_delay_ms(16);  
    }
    
    mouse_hide_cursor();
}

void gui_shutdown(void) {
    running = 0;
    window_count = 0;
    icon_count = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}