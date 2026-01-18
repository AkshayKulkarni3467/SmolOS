#ifndef INCLUDE_SMOLOS_CMDSYS_H
#define INCLUDE_SMOLOS_CMDSYS_H

#include "sos_cmds.h"

void cmd_netinfo_(void);
void cmd_pitinfo(CommandArgs args);
void cmd_benchmark(CommandArgs args);
void cmd_perfmon(CommandArgs args);
void cmd_hdinfo(CommandArgs args);
void cmd_mousetest(CommandArgs args);
void cmd_mousedraw(CommandArgs args);
void cmd_mouse_calibrate(CommandArgs args);
void cmd_mouse_reset_calibration(CommandArgs args);
void cmd_mouse_calibration_info(CommandArgs args);
void cmd_mouseinfo(CommandArgs args);
void cmd_diskinfo_enhanced(CommandArgs args);

#endif //INCLUDE_SMOLOS_CMDSYS_H