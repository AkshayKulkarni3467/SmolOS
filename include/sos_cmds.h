#ifndef INCLUDE_SMOLOS_CMDS_H
#define INCLUDE_SMOLOS_CMDS_H

#include "sos_stdint.h"

#define MAX_ARGS 10
typedef struct {
    char* args[MAX_ARGS];
    int argc;
} CommandArgs;



void print_at_pos(int x, int y, const char* text, uint8_t color);
void print_with_scroll(const char* text);

void run_command(const char* cmd);

#endif // INCLUDE_SMOLOS_CMDS_H