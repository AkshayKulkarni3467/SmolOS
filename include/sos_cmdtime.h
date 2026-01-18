#ifndef INCLUDE_SMOLOS_CMDTIME_H
#define INCLUDE_SMOLOS_CMDTIME_H

#include "sos_cmds.h"

void pit_alarm_callback(void); 
void cmd_setalarm_pit(CommandArgs args);
void cmd_checkalarm_pit(CommandArgs args);
void cmd_time_rtc(CommandArgs args);
void cmd_date(CommandArgs args);
void cmd_datetime(CommandArgs args);
void cmd_uptime_rtc(CommandArgs args);
void cmd_clock(CommandArgs args);
void cmd_timezone(CommandArgs args); 
void cmd_setalarm(CommandArgs args);
void cmd_checkalarm(CommandArgs args);
void cmd_pituptime(CommandArgs args);
void cmd_timer(CommandArgs args) ;
void cmd_stopwatch(CommandArgs args);
void cmd_sleep(CommandArgs args);
void cmd_countdown(CommandArgs args);
void cmd_reaction(CommandArgs args);

#endif //INCLUDE_SMOLOS_CMDTIME_H