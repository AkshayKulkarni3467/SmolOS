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
#include "sos_cmdsys.h"
#include "sos_cmdinfo.h"
#include "sos_cmdtime.h"
#include "sos_cmdgames.h"
#include "sos_cmdgui.h"
#include "sos_cmdnetwork.h"


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

static int games_window_id = -1;
static int games_launcher_active = 0;

static int network_window_id = -1;
static int network_launcher_active = 0;

#define GUI_DESKTOP_BG    VGA_LBLUE
#define GUI_TASKBAR_BG    VGA_DGREY
#define GUI_WINDOW_TITLE  VGA_BLUE
#define GUI_WINDOW_BG     VGA_LGREY

static int point_in_rect(int px, int py, int x, int y, int w, int h) {
    return px >= x && px < x + w && py >= y && py < y + h;
}

static void games_menu_render(int x, int y, int w, int h) {
    vga_set_color(VGA_YELLOW, GUI_WINDOW_BG);
    const char* header = "Select a Game:";
    int len = strlen(header);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + (w - len) / 2, y, header[i]);
    }
    
    const char* games[] = {
        "1. Snake", "2. Tetris",
        "3. Pong", "4. Breakout",
        "5. Minesweeper", "6. 2048",
        "7. Life Sim", "8. Memory",
        "9. Space Shooter", "A. Tic-Tac-Toe",
        "B. Mirror", "C. Type Racer",
        "D. Lunar Lander", "E. Logic Circuit"
    };
    
    int row = 0;
    for (int i = 0; i < 14; i += 2) {
        vga_set_color(VGA_WHITE, GUI_WINDOW_BG);
        
        len = strlen(games[i]);
        for (int j = 0; j < len; j++) {
            vga_putchr_at(x + 2 + j, y + 2 + row, games[i][j]);
        }
        
        if (i + 1 < 14) {
            len = strlen(games[i + 1]);
            for (int j = 0; j < len; j++) {
                vga_putchr_at(x + w/2 + j, y + 2 + row, games[i + 1][j]);
            }
        }
        row++;
    }
    
    vga_set_color(VGA_DGREY, GUI_WINDOW_BG);
    const char* inst = "Press key or ESC to close";
    len = strlen(inst);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + (w - len) / 2, y + h - 2, inst[i]);
    }
}

void gui_app_games_launcher(void) {
    if (games_window_id >= 0) {
        gui_close_window(games_window_id);
        games_window_id = -1;
        games_launcher_active = 0;
        return;
    }
    
    games_window_id = gui_create_window("Games Launcher", 15, 4, 50, 18, GUI_WINDOW_TITLE);
    if (games_window_id >= 0) {
        windows[games_window_id].content_render = games_menu_render;
        games_launcher_active = 1;
    }
}

static void network_menu_render(int x, int y, int w, int h) {
    vga_set_color(VGA_LCYAN, GUI_WINDOW_BG);
    const char* header = "Network Tools";
    int len = strlen(header);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + (w - len) / 2, y, header[i]);
    }
    
    const char* tools[] = {
        "1. DNS Lookup",
        "2. TCP Ping",
        "3. Music Player"
    };
    
    for (int i = 0; i < 3; i++) {
        vga_set_color(VGA_WHITE, GUI_WINDOW_BG);
        len = strlen(tools[i]);
        for (int j = 0; j < len; j++) {
            vga_putchr_at(x + 2 + j, y + 2 + i, tools[i][j]);
        }
    }
    
    vga_set_color(VGA_DGREY, GUI_WINDOW_BG);
    const char* inst = "Press number or ESC";
    len = strlen(inst);
    for (int i = 0; i < len; i++) {
        vga_putchr_at(x + (w - len) / 2, y + h - 2, inst[i]);
    }
}

void gui_app_network_tools(void) {
    if (network_window_id >= 0) {
        gui_close_window(network_window_id);
        network_window_id = -1;
        network_launcher_active = 0;
        return;
    }
    
    network_window_id = gui_create_window("Network Tools", 20, 8, 40, 10, GUI_WINDOW_TITLE);
    if (network_window_id >= 0) {
        windows[network_window_id].content_render = network_menu_render;
        network_launcher_active = 1;
    }
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

void gui_app_snake(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_snake((CommandArgs){0});
    running = 1;
}

void gui_app_tetris(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_tetris((CommandArgs){0});
    running = 1;
}

void gui_app_pong(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_pong((CommandArgs){0});
    running = 1;
}

void gui_app_breakout(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_breakout((CommandArgs){0});
    running = 1;
}

void gui_app_minesweeper(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_minesweeper((CommandArgs){0});
    running = 1;
}

void gui_app_2048(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_2048((CommandArgs){0});
    running = 1;
}

void gui_app_lifesim(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_lifesim((CommandArgs){0});
    running = 1;
}

void gui_app_memorygame(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_memorygame((CommandArgs){0});
    running = 1;
}

void gui_app_spaceshooter(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_spaceshooter((CommandArgs){0});
    running = 1;
}

void gui_app_tictactoe(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_tictactoe((CommandArgs){0});
    running = 1;
}

void gui_app_mirrorgame(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_mirrorgame((CommandArgs){0});
    running = 1;
}

void gui_app_typeracer(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_typeracer((CommandArgs){0});
    running = 1;
}

void gui_app_lunarlander(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_lunarlander((CommandArgs){0});
    running = 1;
}

void gui_app_logiccircuit(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_logiccircuit((CommandArgs){0});
    running = 1;
}

void gui_app_musicplayer(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_musicplayer((CommandArgs){0});
    running = 1;
}

void gui_app_nslookup(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_dns_((CommandArgs){0});
    wait_for_char();
    running = 1;
}

void gui_app_tcpping(void){
    running = 0;
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
    cmd_tcp_ping_((CommandArgs){0});
    wait_for_char();
    running = 1;
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
    games_window_id = -1;
    games_launcher_active = 0;
    network_window_id = -1;
    network_launcher_active = 0;

    gui_add_desktop_icon("Terminal", 0x0F, 42, 8, gui_app_terminal);
    gui_add_desktop_icon("Files", 0xFE, 50, 8, gui_app_file_browser);
    gui_add_desktop_icon("Calc", 0xF7, 58, 8, gui_app_calculator);
    gui_add_desktop_icon("Todo", 0x09, 66, 8, gui_app_todolist);

    gui_add_desktop_icon("Games", 0x02, 42, 11, gui_app_games_launcher);
    gui_add_desktop_icon("Art", 0x0C, 50, 11, gui_app_art_gallery);
    gui_add_desktop_icon("Banner", 0x13, 58, 11, gui_app_banner);
    gui_add_desktop_icon("Draw", 0x04, 66, 11, gui_app_mousedraw);

    gui_add_desktop_icon("Clock", 0x0E, 42, 14, gui_app_clock);
    gui_add_desktop_icon("Timer", 0x07, 50, 14, gui_app_stopwatch);
    gui_add_desktop_icon("Date", 0x0B, 58, 14, gui_app_datetime);
    gui_add_desktop_icon("Uptime", 0x01, 66, 14, gui_app_uptime);

    gui_add_desktop_icon("PerfMon", 0x10, 42, 17, gui_app_perfmon);
    gui_add_desktop_icon("SysInfo", 'i', 50, 17, gui_app_sysinfo);
    gui_add_desktop_icon("Disk", 0xDB, 58, 17, gui_app_diskinfo);
    gui_add_desktop_icon("Bench", 0x0D, 66, 17, gui_app_benchmark);

    gui_add_desktop_icon("Network", 0x12, 42, 20, gui_app_network_tools);
    gui_add_desktop_icon("Mouse", 0x1A, 50, 20, gui_app_mousetest);
    gui_add_desktop_icon("React", 0x05, 58, 20, gui_app_reaction);
    gui_add_desktop_icon("About", '?', 66, 20, gui_app_about);
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

            if (games_launcher_active && games_window_id >= 0 && 
                windows[games_window_id].state != WINDOW_CLOSED &&
                windows[games_window_id].is_focused) {
                
                int launch_game = 0;
                switch (c) {
                    case '1': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_snake(); break;
                    case '2': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_tetris(); break;
                    case '3': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_pong(); break;
                    case '4': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_breakout(); break;
                    case '5': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_minesweeper(); break;
                    case '6': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_2048(); break;
                    case '7': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_lifesim(); break;
                    case '8': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_memorygame(); break;
                    case '9': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_spaceshooter(); break;
                    case 'a': case 'A': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_tictactoe(); break;
                    case 'b': case 'B': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_mirrorgame(); break;
                    case 'c': case 'C': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_typeracer(); break;
                    case 'd': case 'D': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_lunarlander(); break;
                    case 'e': case 'E': launch_game = 1; gui_close_window(games_window_id); games_window_id = -1; games_launcher_active = 0; gui_app_logiccircuit(); break;
                    case 27: 
                        gui_close_window(games_window_id);
                        games_window_id = -1;
                        games_launcher_active = 0;
                        break;
                }
                if (launch_game && !running) {
                    continue;
                }
            }
            else if (network_launcher_active && network_window_id >= 0 && 
                     windows[network_window_id].state != WINDOW_CLOSED &&
                     windows[network_window_id].is_focused) {
                
                switch (c) {
                    case '1': gui_close_window(network_window_id); network_window_id = -1; network_launcher_active = 0; gui_app_nslookup(); break;
                    case '2': gui_close_window(network_window_id); network_window_id = -1; network_launcher_active = 0; gui_app_tcpping(); break;
                    case '3': gui_close_window(network_window_id); network_window_id = -1; network_launcher_active = 0; gui_app_musicplayer(); break;
                    case 27: 
                        gui_close_window(network_window_id);
                        network_window_id = -1;
                        network_launcher_active = 0;
                        break;
                }
                if (!running) {
                    continue;
                }
            }
            else if (c == 27) {  
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
                        if (win->id == games_window_id) {
                            games_window_id = -1;
                            games_launcher_active = 0;
                        } else if (win->id == network_window_id) {
                            network_window_id = -1;
                            network_launcher_active = 0;
                        }
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