#include "sos_cmds.h"
#include "sos_vga.h"
#include "sos_cmdfiles.h"
#include "sos_cmdgames.h"
#include "sos_cmdgui.h"
#include "sos_cmdhelp.h"
#include "sos_cmdinfo.h"
#include "sos_cmdnetwork.h"
#include "sos_cmdshell.h"
#include "sos_cmdsys.h"
#include "sos_cmdtime.h"



static CommandArgs parse_command(char* cmd) {
    CommandArgs result;
    result.argc = 0;
    
    char* token = cmd;
    int in_word = 0;
    
    for (int i = 0; cmd[i] != '\0' && result.argc < MAX_ARGS; i++) {
        if (cmd[i] == ' ' || cmd[i] == '\t') {
            if (in_word) {
                cmd[i] = '\0';
                in_word = 0;
            }
        } else {
            if (!in_word) {
                result.args[result.argc++] = &cmd[i];
                in_word = 1;
            }
        }
    }
    
    return result;
}

void print_at_pos(int x, int y, const char* text, uint8_t color){
    vga_set_color(color, VGA_BLCK);
    for (int i = 0; text[i]; i++) {
        vga_putchr_at(x + i, y, text[i]);
    }
}

void print_with_scroll(const char* text) {
    vga_println((char*)text);
    shell_check_scroll();
}


void run_command(const char* cmd_str) {
    if (cmd_str[0] == '\0') {
        return;
    }
    
    char cmd_copy[256];
    strcpy(cmd_copy, (char*)cmd_str);
    
    CommandArgs args = parse_command(cmd_copy);
    
    if (args.argc == 0) return;
    
    char* cmd = args.args[0];
    
    if (strcmp(cmd, "help") == 0) {
        cmd_help(args);
    }
    else if (strcmp(cmd, "help-sys") == 0) {
        cmd_help_sys(args);
    }
    else if (strcmp(cmd, "help-display") == 0) {
        cmd_help_display(args);
    }
    else if (strcmp(cmd, "help-shell") == 0) {
        cmd_help_shell(args);
    }
    else if (strcmp(cmd, "help-utils") == 0) {
        cmd_help_utils(args);
    }
    else if (strcmp(cmd ,"help-fm") == 0) {
        cmd_help_files(args);
    }
    else if (strcmp(cmd, "about") == 0) {
        cmd_about(args);
    }
    else if (strcmp(cmd, "version") == 0) {
        cmd_version(args);
    }
    else if (strcmp(cmd, "clear") == 0) {
        cmd_clear(args);
    }
    else if (strcmp(cmd, "echo") == 0) {
        cmd_echo(args);
    }
    else if (strcmp(cmd, "color") == 0) {
        cmd_color(args);
    }
    else if (strcmp(cmd, "banner") == 0) {
        cmd_banner(args);
    }
    else if (strcmp(cmd, "rainbow") == 0) {
        cmd_rainbow(args);
    }
    else if (strcmp(cmd, "sysinfo") == 0) {
        cmd_sysinfo(args);
    }
    else if (strcmp(cmd, "calc") == 0) {
        cmd_calc(args);
    }
    else if (strcmp(cmd, "history") == 0) {
        cmd_history(args);
    }
    else if (strcmp(cmd, "clear-history") == 0) {
        cmd_clear_history(args);
    }
    else if (strcmp(cmd, "username") == 0) {
        cmd_username(args);
    }
    else if (strcmp(cmd, "files") == 0){
        file_manager_command();
    }
    else if (strcmp(cmd, "files") == 0 || strcmp(cmd, "fm") == 0) {
        cmd_files(args);
    }
    else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0) {
        cmd_ls(args);
    }
    else if (strcmp(cmd, "cat") == 0) {
        cmd_cat(args);
    }
    else if (strcmp(cmd, "touch") == 0) {
        cmd_touch(args);
    }
    else if (strcmp(cmd, "rm") == 0) {
        cmd_rm(args);
    }
    else if (strcmp(cmd, "hdinfo") == 0 || strcmp(cmd, "disk") == 0) {
        cmd_hdinfo(args);
    }
    else if (strcmp(cmd, "diskinfo") == 0){
        cmd_diskinfo_enhanced(args);
    }
    else if (strcmp(cmd, "calc-gui") == 0){
        calc_command();
    }
    else if (strcmp(cmd, "time") == 0) {
        cmd_time_rtc(args);
    }
    else if (strcmp(cmd, "date") == 0) {
        cmd_date(args);
    }
    else if (strcmp(cmd, "datetime") == 0) {
        cmd_datetime(args);
    }
    else if (strcmp(cmd, "clock") == 0) {
        cmd_clock(args);
    }
    else if (strcmp(cmd, "uptime") == 0) {
        cmd_uptime_rtc(args);
    }
    else if (strcmp(cmd, "timezone") == 0) {
        cmd_timezone(args);
    }
    else if (strcmp(cmd, "setalarm") == 0) {
        cmd_setalarm(args);
    }
    else if (strcmp(cmd, "checkalarm") == 0) {
        cmd_checkalarm(args);
    }
    else if (strcmp(cmd, "pitinfo") == 0) {
        cmd_pitinfo(args);
    }
    else if (strcmp(cmd, "pituptime") == 0) {
        cmd_pituptime(args);
    }
    else if (strcmp(cmd, "benchmark") == 0) {
        cmd_benchmark(args);
    }
    else if (strcmp(cmd, "sleep") == 0) {
        cmd_sleep(args);
    }
    else if (strcmp(cmd, "timer") == 0) {
        cmd_timer(args);
    }
    else if (strcmp(cmd, "countdown") == 0) {
        cmd_countdown(args);
    }
    else if (strcmp(cmd, "stopwatch") == 0) {
        cmd_stopwatch(args);
    }
    else if (strcmp(cmd, "perfmon") == 0) {
        cmd_perfmon(args);
    }
    else if (strcmp(cmd, "setalarm-pit") == 0) {
        cmd_setalarm_pit(args);
    }
    else if (strcmp(cmd, "checkalarm-pit") == 0) {
        cmd_checkalarm_pit(args);
    }
    else if (strcmp(cmd, "reaction") == 0) {
        cmd_reaction(args);
    }
    else if (strcmp(cmd, "todo") == 0) {
        cmd_todo();
    }
    else if (strcmp(cmd, "mousetest") == 0) {
        cmd_mousetest(args);
    }
    else if (strcmp(cmd, "mousedraw") == 0) {
        cmd_mousedraw(args);
    }
    else if (strcmp(cmd, "mouseinfo") == 0) {
        cmd_mouseinfo(args);
    }
    else if (strcmp(cmd, "shutdown") == 0 || strcmp(cmd, "exit") == 0) {
        shutdown();
    }
    else if (strcmp(cmd, "reboot") == 0){
        reboot();
    }
    else if (strcmp(cmd, "artgallery") == 0) {
        art_gallery_main();
    }
    else if (strcmp(cmd, "gui") == 0) {
        cmd_gui(args);
    }
    else if (strcmp(cmd, "snake") == 0) {
        cmd_snake(args);
    }
    else if (strcmp(cmd,"tetris") == 0){
        cmd_tetris(args);
    }
    else if (strcmp(cmd,"pong") == 0){
        cmd_pong(args);
    }
    else if (strcmp(cmd,"breakout") == 0){
        cmd_breakout(args);
    }
    else if (strcmp(cmd,"minesweeper") == 0){
        cmd_minesweeper(args);
    }
    else if (strcmp(cmd,"2048") == 0){
        cmd_2048(args);
    }
    else if (strcmp(cmd,"lifesim") == 0){
        cmd_lifesim(args);
    }
    else if (strcmp(cmd,"memorygame") == 0){
        cmd_memorygame(args);
    }
    else if (strcmp(cmd, "spaceshooter") == 0){
        cmd_spaceshooter(args);
    }
    else if (strcmp(cmd,"tictactoe") == 0){
        cmd_tictactoe(args);
    }
    else if (strcmp(cmd,"mirrorgame") == 0){
        cmd_mirrorgame(args);
    }
    else if(strcmp(cmd,"typeracer") == 0){
        cmd_typeracer(args);
    }
    else if(strcmp(cmd,"lunarlander") == 0){
        cmd_lunarlander(args);
    }
    else if(strcmp(cmd,"logiccircuit") == 0){
        run_logiccircuit_game();
    }
    else if (strcmp(cmd,"musicplayer") == 0){
        cmd_musicplayer(args);
    }
    else if (strcmp(cmd, "pciinfo") == 0){
        cmd_pciinfo();
    }
    else if (strcmp(cmd, "lspci") == 0){
        cmd_lspci();
    }
    else if (strcmp(cmd, "netinfo") == 0){
        cmd_netinfo_();
    }
    else if (strcmp(cmd, "ping") == 0){
        cmd_ping_(args);
    }
    else if (strcmp(cmd,"sendudp") == 0){
        cmd_udp_send_(args);
    }
    else if (strcmp(cmd,"nslookup") == 0){
        cmd_dns_(args);
    }
    else if(strcmp(cmd, "tcpping") == 0){
        cmd_tcp_ping_(args);
    }
    else if (strcmp(cmd, "mousecalibrate") == 0 || strcmp(cmd, "mouse-cal") == 0) {
        cmd_mouse_calibrate(args);
    }
    else if (strcmp(cmd, "mouse-reset-cal") == 0) {
        cmd_mouse_reset_calibration(args);
    }
    else if (strcmp(cmd, "mouse-cal-info") == 0) {
        cmd_mouse_calibration_info(args);
    }
    else {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_print("Unknown command: '");
        vga_print(cmd);
        vga_println("'");
        shell_check_scroll();
        vga_set_color(VGA_LGREY, VGA_BLCK);
        print_with_scroll("Type 'help' for available commands");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    }
}