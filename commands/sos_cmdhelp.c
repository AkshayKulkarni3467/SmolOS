#include "sos_cmdhelp.h"
#include "sos_cmds.h"
#include "sos_vga.h"

void cmd_help_files(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-files       - Show this help message");
    print_with_scroll("  ls               - List files");
    print_with_scroll("  cat              - Display contents of file");
    print_with_scroll("  touch            - Create a file");
    print_with_scroll("  rm               - Delete a file");
    print_with_scroll("  files            - Open file manager");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_games(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-games       - Show this help message");
    print_with_scroll("  logiccircuit     - Create digital circuits!");
    print_with_scroll("  lunarlander      - Play with physics and gravity!");
    print_with_scroll("  typeracer        - Display your typing skills!");
    print_with_scroll("  mirrorgame       - Maze like game with lasers!");
    print_with_scroll("  tictactoe        - Basic TicTacToe");
    print_with_scroll("  spaceshooter     - Peew peew!");
    print_with_scroll("  memorygame       - Play Memory game");
    print_with_scroll("  lifesim          - Fun conway's game of life simulator");
    print_with_scroll("  2048             - Play 2048 game");
    print_with_scroll("  minesweeper      - Play minesweeper");
    print_with_scroll("  breakout         - Play breakout");
    print_with_scroll("  pong             - Play pong");
    print_with_scroll("  snake            - Play snake and apple");
    print_with_scroll("  tetris           - Play tetris");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_gui(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-gui         - Show this help message");
    print_with_scroll("  gui              - Demo of GUI");
    print_with_scroll("  files            - Open file manager");
    print_with_scroll("  musicplayer      - Open music player");
    print_with_scroll("  artgallery       - Open art gallery");
    print_with_scroll("  todo             - Open todolist");
    print_with_scroll("  calc-gui         - Open calculator");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_info(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-info        - Show this help message");
    print_with_scroll("  about            - About the OS");
    print_with_scroll("  banner           - Banner of OS");
    print_with_scroll("  version          - OS Version");
    print_with_scroll("  sysinfo          - System information");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_network(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-network     - Show this help message");
    print_with_scroll("  ping             - Demo of ping");
    print_with_scroll("  sendudp          - Send UDP to any address");
    print_with_scroll("  nslookup         - Lookup IP of a Domain");
    print_with_scroll("  tcpping          - Check if an IP is running");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_shell(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-shell       - Show this help message");
    print_with_scroll("  echo             - echo text");
    print_with_scroll("  color            - change fg and bg color");
    print_with_scroll("  rainbow          - Display rainbow text");
    print_with_scroll("  calc             - Basic calculator parser");
    print_with_scroll("  history          - Display command history");
    print_with_scroll("  clear-history    - Clear command history");
    print_with_scroll("  username         - Change username");
    print_with_scroll("  clear            - Clear the shell");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_sys(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-sys         - Show this help message");
    print_with_scroll("  netinfo          - Display network information");
    print_with_scroll("  pitinfo          - Display pit timer information");
    print_with_scroll("  benchmark        - Display OS benchmark");
    print_with_scroll("  perfmon          - Display performance monitor");
    print_with_scroll("  hdinfo           - Display disk info");
    print_with_scroll("  pciinfo          - Display PCI device browser");
    print_with_scroll("  lspci            - Display PCI enumeration");
    print_with_scroll("  diskinfo         - Display disk usage");
    print_with_scroll("  mouseinfo        - Display mouse information");
    print_with_scroll("  mousetest        - Test mouse cursor");
    print_with_scroll("  mousedraw        - Draw using mouse");
    print_with_scroll("  mousecalibrate   - Calibrate your mouse");
    print_with_scroll("  mouse-cal-info   - Mouse calibration information");
    print_with_scroll("  mouse-reset-cal  - Reset mouse calibration");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help_time(CommandArgs args){
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help-time        - Show this help message");
    print_with_scroll("  reaction         - Check your reaction speed!");
    print_with_scroll("  countdown        - Countdown from a number");
    print_with_scroll("  sleep            - sleep <seconds>");
    print_with_scroll("  stopwatch        - Enable stopwatch");
    print_with_scroll("  timer            - Enable timer");
    print_with_scroll("  pituptime        - Display uptime of OS (Uses PIT)");
    print_with_scroll("  checkalarm       - Check if alarm went off or not (RTC)");
    print_with_scroll("  setalarm         - Set an alarm (Time of day)");
    print_with_scroll("  timezone         - Check current timezone");
    print_with_scroll("  clock            - Display clock");
    print_with_scroll("  uptime           - Display uptime of OS (Uses RTC)");
    print_with_scroll("  datetime         - Display date and time");
    print_with_scroll("  date             - Display date");
    print_with_scroll("  time             - Display time");
    print_with_scroll("  setalarm-pit     - Set an alarm (N seconds after enabling)");
    print_with_scroll("  checkalarm-pit   - Cehck if alarm went off or not (PIT)");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_help(CommandArgs args) {
    vga_set_color(VGA_LGREY, VGA_BLCK);
    print_with_scroll("  help             - Show this help message");
    print_with_scroll("  help-files       - Display file commands");
    print_with_scroll("  help-games       - Show games");
    print_with_scroll("  help-gui         - Display GUI commands");
    print_with_scroll("  help-info        - Display Info commands");
    print_with_scroll("  help-network     - Display Network commands");
    print_with_scroll("  help-shell       - Display Shell commands");
    print_with_scroll("  help-sys         - Display System commands");
    print_with_scroll("  help-time        - Display Time commands");
    print_with_scroll("  clear            - Clear the shell");
    print_with_scroll("  reboot           - Reboots the OS");
    print_with_scroll("  exit             - Exit the shell");
    print_with_scroll("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}