#ifndef INCLUDE_SMOLOS_CMDS_H
#define INCLUDE_SMOLOS_CMDS_H

#define MAX_ARGS 10
typedef struct {
    char* args[MAX_ARGS];
    int argc;
} CommandArgs;

void cmd_about(CommandArgs args);
void cmd_sysinfo(CommandArgs args);
void cmd_clock(CommandArgs args);
void cmd_banner(CommandArgs args);
void cmd_reaction(CommandArgs args);
void cmd_perfmon(CommandArgs args);
void cmd_stopwatch(CommandArgs args);
void cmd_datetime(CommandArgs args);
void cmd_mousedraw(CommandArgs args);
void cmd_mousetest(CommandArgs args);

void run_command(const char* cmd);

#endif // INCLUDE_SMOLOS_CMDS_H