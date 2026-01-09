#ifndef INCLUDE_SMOLOS_SHELL_H
#define INCLUDE_SMOLOS_SHELL_H

#include "sos_stdint.h"

static const char* startup_text[] = {
    "  _____                 _ ____   _____ ",
    " / ____|               | / __ \\ / ____|",
    "| (___  _ __ ___   ___ | | |  | | (___  ",
    " \\___ \\| '_ ` _ \\ / _ \\| | |  | |\\___ \\ ",
    " ____) | | | | | | (_) | | |__| |____) |",
    "|_____/|_| |_| |_|\\___/|_|\\____/|_____/ ",
    "",
    "             Welcome to SmolOS V1.0       ",
    0
    };

extern int scroll_enabled;


void shell_init(void);

void shell_prompt(void);
void run_shell(void);

void shell_display_startup(void);
int shell_is_running(void);
void shell_exit(void);


void shell_check_scroll(void);
void shell_set_prompt_color(uint8_t color);
void shell_set_input_color(uint8_t color);


char* shell_get_directory(void);
char* shell_get_username(void);
void shell_set_directory(const char* dir);
void shell_set_username(const char* name);

void shell_add_to_history(const char* cmd);
void shell_show_history(void);
void shell_clear_history(void);


void shell_print_error(const char* msg);
void shell_print_success(const char* msg);
void shell_print_info(const char* msg);

#endif // INCLUDE_SMOLOS_SHELL_H