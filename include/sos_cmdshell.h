#ifndef INCLUDE_SMOLOS_CMDSHELL_H
#define INCLUDE_SMOLOS_CMDSHELL_H

#include "sos_cmds.h"

void cmd_echo(CommandArgs args);
void cmd_color(CommandArgs args); 
void cmd_rainbow(CommandArgs args);
void cmd_calc(CommandArgs args);
void cmd_history(CommandArgs args);
void cmd_clear_history(CommandArgs args);
void cmd_username(CommandArgs args);
void cmd_clear(CommandArgs args);

#endif //INCLUDE_SMOLOS_CMDSHELL_H