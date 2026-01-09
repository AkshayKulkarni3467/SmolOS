#ifndef INCLUDE_SMOLOS_SHELL_H
#define INCLUDE_SMOLOS_SHELL_H

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

void run_shell(void);

#endif //INCLUDE_SMOLOS_SHELL_H