#include "sos_cmdtime.h"
#include "sos_cmds.h"
#include "sos_rtc.h"
#include "sos_pit.h"
#include "sos_vga.h"

static volatile uint32_t pit_alarm_target_seconds = 0;
static volatile int pit_alarm_triggered = 0;

void pit_alarm_callback(void) {

    
    if (pit_alarm_target_seconds > 0) {
        uint32_t current_seconds = pit_get_seconds();
        
        if (current_seconds >= pit_alarm_target_seconds) {
            pit_alarm_triggered = 1;
            pit_alarm_target_seconds = 0; 
        }
    }
}

void cmd_setalarm_pit(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: setalarm-pit <seconds>");
        print_with_scroll("Example: setalarm-pit 60   (alarm in 60 seconds)");
        return;
    }
    
    uint32_t seconds = 0;
    int i = 0;
    while (args.args[1][i]) {
        if (args.args[1][i] >= '0' && args.args[1][i] <= '9') {
            seconds = seconds * 10 + (args.args[1][i] - '0');
        }
        i++;
    }
    
    if (seconds == 0 || seconds > 86400) {  
        shell_print_error("Invalid duration (1-86400 seconds)");
        return;
    }
    
    uint32_t current_seconds = pit_get_seconds();
    pit_alarm_target_seconds = current_seconds + seconds;
    pit_alarm_triggered = 0;
    
    pit_register_callback(pit_alarm_callback);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_print("PIT alarm set for ");
    vga_print_int(seconds);
    vga_println(" seconds from now");
    shell_check_scroll();
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print("(Alarm will trigger at system second ");
    vga_print_int(pit_alarm_target_seconds);
    vga_println(")");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll("Use 'checkalarm-pit' to check alarm status");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_checkalarm_pit(CommandArgs args) {
    if (pit_alarm_triggered) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_println("");
        vga_println("  *** PIT ALARM TRIGGERED! ***");
        vga_println("");
        shell_check_scroll();
        
        int alarm_start_row = vga_t_row;
        
        for (int i = 0; i < 5; i++) {
            vga_fill_rect(0, alarm_start_row, 80, 3, ' ', VGA_YELLOW, VGA_RED);
            
            vga_t_row = alarm_start_row + 1;
            vga_t_column = 0;
            vga_setcursor(0, alarm_start_row + 1);
            
            vga_set_color(VGA_YELLOW, VGA_RED);
            vga_print_centered("*** ALARM! WAKE UP! ***", alarm_start_row + 1);
            pit_delay_ms(200);
            
            vga_fill_rect(0, alarm_start_row, 80, 3, ' ', VGA_WHITE, VGA_BLCK);
            pit_delay_ms(200);
        }
        
        pit_alarm_triggered = 0;
        pit_alarm_target_seconds = 0;
        pit_unregister_callback();
        
        vga_t_row = alarm_start_row + 3;
        vga_t_column = 0;
        vga_setcursor(0, vga_t_row);
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        print_with_scroll("Alarm acknowledged and cleared.");
    } else if (pit_alarm_target_seconds > 0) {
        uint32_t current_seconds = pit_get_seconds();
        
        if (current_seconds < pit_alarm_target_seconds) {
            uint32_t remaining = pit_alarm_target_seconds - current_seconds;
            
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            vga_print("Alarm pending: ");
            vga_print_int(remaining);
            vga_println(" seconds remaining");
            shell_check_scroll();
        } else {
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            print_with_scroll("Alarm should trigger any moment now...");
        }
    } else {
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        print_with_scroll("No PIT alarm set.");
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_time_rtc(CommandArgs args) {
    rtc_init();
    
    RTCTime time;
    rtc_get_local_time(&time);  
    
    char time_str[32];
    rtc_format_time(&time, time_str);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("Current Time: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll(time_str);
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_print("                (");
    vga_print((char*)rtc_get_weekday_name(time.weekday));
    vga_println(")");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_date(CommandArgs args) {
    rtc_init();
    
    RTCTime time;
    rtc_get_local_time(&time);  
    
    char date_str[32];
    rtc_format_date(&time, date_str);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("Current Date: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll(date_str);
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_print("                (");
    vga_print((char*)rtc_get_weekday_name(time.weekday));
    vga_print(", ");
    vga_print((char*)rtc_get_month_name(time.month));
    vga_println(")");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_datetime(CommandArgs args) {
    rtc_init();
    vga_clear();
    vga_begin_batch();
    
    vga_draw_box_double(15, 8, 50, 8, VGA_CYAN, VGA_BLCK);
    
    RTCTime time;
    rtc_get_local_time(&time);  
    
    char time_str[32];
    char date_str[32];
    rtc_format_time(&time, time_str);
    rtc_format_date(&time, date_str);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_at(25, 9, "=== Date & Time ===", VGA_YELLOW, VGA_BLCK);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_at(18, 11, "Date:  ", VGA_LCYAN, VGA_BLCK);
    print_at(25, 11, date_str, VGA_WHITE, VGA_BLCK);
    
    print_at(18, 12, "Time:  ", VGA_LCYAN, VGA_BLCK);
    print_at(25, 12, time_str, VGA_WHITE, VGA_BLCK);
    
    print_at(18, 13, "Day:   ", VGA_LCYAN, VGA_BLCK);
    print_at(25, 13, rtc_get_weekday_name(time.weekday), VGA_WHITE, VGA_BLCK);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    print_at(22, 15, "Press any key to continue...", VGA_DGREY, VGA_BLCK);
    
    vga_end_batch();
    
    wait_for_char();
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_uptime_rtc(CommandArgs args) {
    uint32_t uptime = rtc_get_uptime_seconds();
    
    if (uptime == 0) {
        extern uint32_t rtc_get_interrupt_count(void);
        uint32_t int_count = rtc_get_interrupt_count();
        
        if (int_count == 0) {
            vga_set_color(VGA_LRED, VGA_BLCK);
            print_with_scroll("Uptime: 0 (RTC interrupts not working)");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            return;
        }
    }
    
    char uptime_str[64];
    rtc_format_uptime(uptime_str);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("System Uptime: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll(uptime_str);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


void cmd_clock(CommandArgs args) {
    rtc_init();
    
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("=== Live Clock ===", 2);
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Press ESC to exit", 23);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    vga_draw_box_double(10, 6, 60, 12, VGA_CYAN, VGA_BLCK);
    
    vga_end_batch();
    
    int running = 1;
    int counter = 0;
    
    while (running) {
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            if (c == 27) {  
                running = 0;
            }
        }
        
        if (counter % 1000 == 0) {
            RTCTime time;
            rtc_get_local_time(&time);  
            
            char time_str[32];
            char date_str[32];
            rtc_format_time(&time, time_str);
            rtc_format_date(&time, date_str);
            
            vga_begin_batch();
            
            vga_fill_rect(12, 8, 56, 8, ' ', VGA_WHITE, VGA_BLCK);
            
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            print_at(28, 10, time_str, VGA_YELLOW, VGA_BLCK);
            
            vga_set_color(VGA_LCYAN, VGA_BLCK);
            print_at(27, 12, date_str, VGA_LCYAN, VGA_BLCK);
            
            vga_set_color(VGA_WHITE, VGA_BLCK);
            const char* weekday = rtc_get_weekday_name(time.weekday);
            int weekday_len = strlen((char*)weekday);
            int weekday_x = 40 - (weekday_len / 2);
            print_at(weekday_x, 14, weekday, VGA_WHITE, VGA_BLCK);
            
            vga_end_batch();
        }
        
        counter++;
        
        for (volatile int i = 0; i < 1000; i++) {
            asm volatile("nop");
        }
    }
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_timezone(CommandArgs args) {
    if (args.argc < 2) {
        int hours, minutes;
        rtc_get_timezone_offset(&hours, &minutes);
        
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        vga_print("Current Timezone: UTC");
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        
        if (hours >= 0 && minutes >= 0) {
            vga_print("+");
        }
        vga_print_int(hours);
        vga_print(":");
        if (minutes < 10) vga_print("0");
        vga_print_int(minutes);
        vga_println("");
        shell_check_scroll();
        
        vga_set_color(VGA_LGREY, VGA_BLCK);
        print_with_scroll("");
        print_with_scroll("Usage: timezone <hours> <minutes>");
        print_with_scroll("Example: timezone 5 30      (for IST)");
        print_with_scroll("Example: timezone -5 0      (for EST)");
        print_with_scroll("");
        print_with_scroll("Common Timezones:");
        print_with_scroll("  IST  (India):      +5:30");
        print_with_scroll("  EST  (US East):    -5:00");
        print_with_scroll("  PST  (US West):    -8:00");
        print_with_scroll("  GMT/UTC:            0:00");
        print_with_scroll("  JST  (Japan):      +9:00");
        print_with_scroll("  AEST (Australia): +10:00");
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        return;
    }
    
    if (args.argc < 3) {
        shell_print_error("Usage: timezone <hours> <minutes>");
        return;
    }
    
    int hours = 0, minutes = 0;
    int negative_hours = 0;
    
    int i = 0;
    if (args.args[1][0] == '-') {
        negative_hours = 1;
        i = 1;
    }
    
    for (; args.args[1][i]; i++) {
        if (args.args[1][i] >= '0' && args.args[1][i] <= '9') {
            hours = hours * 10 + (args.args[1][i] - '0');
        }
    }
    
    if (negative_hours) hours = -hours;
    
    for (i = 0; args.args[2][i]; i++) {
        if (args.args[2][i] >= '0' && args.args[2][i] <= '9') {
            minutes = minutes * 10 + (args.args[2][i] - '0');
        }
    }
    
    if (hours < -12 || hours > 14 || minutes < 0 || minutes > 59) {
        shell_print_error("Invalid timezone! Hours: -12 to +14, Minutes: 0-59");
        return;
    }
    
    rtc_set_timezone_offset(hours, minutes);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_print("Timezone set to UTC");
    if (hours >= 0 && minutes >= 0) {
        vga_print("+");
    }
    vga_print_int(hours);
    vga_print(":");
    if (minutes < 10) vga_print("0");
    vga_print_int(minutes);
    vga_println("");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_setalarm(CommandArgs args) {
    if (args.argc < 4) {
        shell_print_error("Usage: setalarm <hours> <minutes> <seconds>");
        print_with_scroll("Example: setalarm 14 30 0");
        return;
    }
    
    int hours = 0, minutes = 0, seconds = 0;
    
    for (int i = 0; args.args[1][i]; i++) {
        if (args.args[1][i] >= '0' && args.args[1][i] <= '9') {
            hours = hours * 10 + (args.args[1][i] - '0');
        }
    }
    
    for (int i = 0; args.args[2][i]; i++) {
        if (args.args[2][i] >= '0' && args.args[2][i] <= '9') {
            minutes = minutes * 10 + (args.args[2][i] - '0');
        }
    }
    
    for (int i = 0; args.args[3][i]; i++) {
        if (args.args[3][i] >= '0' && args.args[3][i] <= '9') {
            seconds = seconds * 10 + (args.args[3][i] - '0');
        }
    }
    
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59 || seconds < 0 || seconds > 59) {
        shell_print_error("Invalid time! Use 24-hour format.");
        return;
    }
    
    rtc_init();
    rtc_set_alarm(hours, minutes, seconds);
    rtc_enable_alarm();
    rtc_clear_alarm();  
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_print("Alarm set for: ");
    vga_print_int(hours);
    vga_print(":");
    if (minutes < 10) vga_print("0");
    vga_print_int(minutes);
    vga_print(":");
    if (seconds < 10) vga_print("0");
    vga_print_int(seconds);
    vga_println("");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll("Note: Alarm will beep and display message when time is reached.");
    print_with_scroll("Use 'checkalarm' to see if alarm has triggered.");
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_checkalarm(CommandArgs args) {
    if (rtc_alarm_triggered()) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_println("");
        vga_println("  *** ALARM TRIGGERED! ***");
        vga_println("  *** WAKE UP! ***");
        vga_println("");
        shell_check_scroll();
        
        for (int i = 0; i < 3; i++) {
            vga_fill_rect(0, 10, 80, 5, ' ', VGA_YELLOW, VGA_RED);
            vga_set_color(VGA_YELLOW, VGA_RED);
            vga_print_centered("*** ALARM! ALARM! ALARM! ***", 12);
            
            for (volatile int j = 0; j < 1000000; j++);
            
            vga_fill_rect(0, 10, 80, 5, ' ', VGA_WHITE, VGA_BLCK);
            
            for (volatile int j = 0; j < 1000000; j++);
        }
        
        rtc_clear_alarm();
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        print_with_scroll("Alarm acknowledged and cleared.");
    } else {
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        print_with_scroll("No alarm triggered.");
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_pituptime(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print("PIT Uptime: ");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    
    char uptime_str[64];
    pit_format_uptime(uptime_str);
    print_with_scroll(uptime_str);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print("(");
    vga_print_int(pit_get_seconds());
    vga_print(" seconds, ");
    vga_print_int(pit_get_milliseconds());
    vga_println(" ms)");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_timer(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== Performance Timer Demo ===");
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll("Testing timer accuracy...");
    print_with_scroll("");
    
    uint32_t test_delays[] = {1, 10, 50, 100, 500, 1000};
    
    for (int i = 0; i < 6; i++) {
        uint32_t target = test_delays[i];
        
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_print("Target delay: ");
        vga_print_int(target);
        vga_print(" ms -> ");
        
        pit_start_timer();
        pit_delay_ms(target);
        uint32_t elapsed_ticks = pit_stop_timer();
        
        uint32_t elapsed_ms = (elapsed_ticks * 1000) / pit_get_frequency();
        
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        vga_print("Actual: ");
        vga_print_int(elapsed_ms);
        vga_print(" ms (");
        vga_print_int(elapsed_ticks);
        vga_println(" ticks)");
        shell_check_scroll();
    }
    
    print_with_scroll("");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    print_with_scroll("Timer test complete!");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}


void cmd_stopwatch(CommandArgs args) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(20, 5, 40, 12, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("=== Stopwatch ===", 6);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("SPACE = Start/Stop", 15);
    vga_print_centered("R = Reset, ESC = Exit", 16);
    
    vga_end_batch();
    
    int running = 0;
    uint64_t start_ticks = 0;
    uint64_t elapsed_ticks = 0;
    uint64_t paused_elapsed = 0;
    
    while (1) {
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            
            if (c == ' ') {  
                if (!running) {
                    start_ticks = pit_get_ticks();
                    running = 1;
                } else {
                    paused_elapsed = elapsed_ticks;
                    running = 0;
                }
            }
            else if (c == 'r' || c == 'R') {  
                running = 0;
                elapsed_ticks = 0;
                paused_elapsed = 0;
            }
            else if (c == 27) {  
                break;
            }
        }
        
        if (running) {
            elapsed_ticks = paused_elapsed + (pit_get_ticks() - start_ticks);
        } else {
            elapsed_ticks = paused_elapsed;
        }
        
        uint32_t total_ms = (uint32_t)((elapsed_ticks * 1000) / pit_get_frequency());
        uint32_t hours = total_ms / 3600000;
        total_ms %= 3600000;
        uint32_t minutes = total_ms / 60000;
        total_ms %= 60000;
        uint32_t seconds = total_ms / 1000;
        uint32_t milliseconds = total_ms % 1000;
        
        vga_begin_batch();
        
        vga_fill_rect(22, 8, 36, 5, ' ', VGA_WHITE, VGA_BLCK);
        
        char time_str[32];
        int pos = 0;
        
        time_str[pos++] = '0' + (hours / 10);
        time_str[pos++] = '0' + (hours % 10);
        time_str[pos++] = ':';
        
        time_str[pos++] = '0' + (minutes / 10);
        time_str[pos++] = '0' + (minutes % 10);
        time_str[pos++] = ':';
        
        time_str[pos++] = '0' + (seconds / 10);
        time_str[pos++] = '0' + (seconds % 10);
        time_str[pos++] = '.';
        
        time_str[pos++] = '0' + (milliseconds / 100);
        time_str[pos++] = '0' + ((milliseconds / 10) % 10);
        time_str[pos++] = '0' + (milliseconds % 10);
        time_str[pos] = '\0';
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        print_at(28, 10, time_str, VGA_LGREEN, VGA_BLCK);
        
        if (running) {
            vga_set_color(VGA_LRED, VGA_BLCK);
            print_at(38, 10, "[RUNNING]", VGA_LRED, VGA_BLCK);
        } else {
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            print_at(38, 10, "[PAUSED] ", VGA_YELLOW, VGA_BLCK);
        }
        
        vga_end_batch();
        
        pit_delay_ms(10);
    }
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}




void cmd_sleep(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: sleep <milliseconds>");
        print_with_scroll("Example: sleep 1000   (sleep for 1 second)");
        return;
    }
    
    uint32_t ms = 0;
    int i = 0;
    while (args.args[1][i]) {
        if (args.args[1][i] >= '0' && args.args[1][i] <= '9') {
            ms = ms * 10 + (args.args[1][i] - '0');
        }
        i++;
    }
    
    if (ms == 0 || ms > 60000) {
        shell_print_error("Invalid duration (1-60000 ms)");
        return;
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Sleeping for ");
    vga_print_int(ms);
    vga_println(" ms...");
    shell_check_scroll();
    
    int bar_row = vga_t_row;
    int bar_width = 40;
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print("[");
    for (int i = 0; i < bar_width; i++) vga_putchr(' ');
    vga_println("]");
    shell_check_scroll();
    
    uint32_t steps = 20;
    uint32_t step_ms = ms / steps;
    
    for (uint32_t step = 0; step < steps; step++) {
        pit_delay_ms(step_ms);
        
        int filled = (bar_width * (step + 1)) / steps;
        
        vga_begin_batch();
        
        for (int j = 0; j < bar_width; j++) {
            int screen_x = 1 + j; 
            int screen_y = bar_row;
            
            if (j < filled) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, '#');
            } else {
                vga_set_color(VGA_DGREY, VGA_BLCK);
                vga_putchr_at(screen_x, screen_y, ' ');
            }
        }
        
        vga_end_batch();
    }
    
    vga_t_row = bar_row + 1;
    vga_t_column = 0;
    vga_setcursor(0, vga_t_row);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    print_with_scroll("Done!");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_countdown(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: countdown <seconds>");
        print_with_scroll("Example: countdown 10");
        return;
    }
    
    uint32_t seconds = 0;
    int i = 0;
    while (args.args[1][i]) {
        if (args.args[1][i] >= '0' && args.args[1][i] <= '9') {
            seconds = seconds * 10 + (args.args[1][i] - '0');
        }
        i++;
    }
    
    if (seconds == 0 || seconds > 3600) {
        shell_print_error("Invalid duration (1-3600 seconds)");
        return;
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== Countdown ===");
    print_with_scroll("");
    
    int countdown_row = vga_t_row;
    
    vga_println("");
    
    for (uint32_t i = seconds; i > 0; i--) {
        vga_begin_batch();
        
        vga_fill_rect(0, countdown_row, WIDTH, 1, ' ', VGA_WHITE, VGA_BLCK);
        
        char countdown_text[80];
        int pos = 0;
        
        const char* prefix = "Time remaining: ";
        for (int j = 0; prefix[j]; j++) {
            countdown_text[pos++] = prefix[j];
        }
        
        if (i >= 10) {
            countdown_text[pos++] = '0' + (i / 10);
            countdown_text[pos++] = '0' + (i % 10);
        } else {
            countdown_text[pos++] = '0';
            countdown_text[pos++] = '0' + i;
        }
        
        const char* suffix = " seconds";
        for (int j = 0; suffix[j]; j++) {
            countdown_text[pos++] = suffix[j];
        }
        countdown_text[pos] = '\0';
        
        int x = 0;
        for (int j = 0; countdown_text[j]; j++) {
            uint8_t color = i <= 5 ? VGA_LRED : VGA_LGREEN;
            if (j < 16) {  
                color = VGA_YELLOW;
            }
            vga_set_color(color, VGA_BLCK);
            vga_putchr_at(x++, countdown_row, countdown_text[j]);
        }
        
        vga_end_batch();
        
        pit_delay_ms(1000);
    }
    
    vga_begin_batch();
    vga_fill_rect(0, countdown_row, WIDTH, 1, ' ', VGA_WHITE, VGA_BLCK);
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    int x = 0;
    const char* msg = "Time's up!";
    for (int j = 0; msg[j]; j++) {
        vga_putchr_at(x++, countdown_row, msg[j]);
    }
    vga_end_batch();
    
    vga_t_row = countdown_row + 1;
    vga_t_column = 0;
    vga_setcursor(0, vga_t_row);
    
    int flash_row = vga_t_row;
    vga_println("");  
    
    for (int flash = 0; flash < 3; flash++) {
        vga_begin_batch();
        vga_fill_rect(0, flash_row, 13, 1, ' ', VGA_YELLOW, VGA_RED);
        const char* done_msg = "*** DONE! ***";
        for (int j = 0; done_msg[j]; j++) {
            vga_set_color(VGA_YELLOW, VGA_RED);
            vga_putchr_at(j, flash_row, done_msg[j]);
        }
        vga_end_batch();
        
        pit_delay_ms(200);
        
        vga_begin_batch();
        vga_fill_rect(0, flash_row, 13, 1, ' ', VGA_WHITE, VGA_BLCK);
        vga_end_batch();
        
        pit_delay_ms(200);
    }
    
    vga_begin_batch();
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    const char* done_msg = "*** DONE! ***";
    for (int j = 0; done_msg[j]; j++) {
        vga_putchr_at(j, flash_row, done_msg[j]);
    }
    vga_end_batch();
    
    vga_t_row = flash_row + 1;
    vga_t_column = 0;
    vga_setcursor(0, vga_t_row);
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_reaction(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== Reaction Time Test ===");
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll("Press SPACE as quickly as possible when you see GO!");
    print_with_scroll("Starting in 3 seconds...");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    pit_delay_ms(3000);
    
    uint32_t random_delay = 1000 + (pit_get_ticks() % 3000);
    pit_delay_ms(random_delay);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("");
    vga_println("        GO!!!");
    shell_check_scroll();
    
    pit_start_timer();
    
    while (1) {
        keyboard_poll();
        if (has_key() && get_char() == ' ') {
            break;
        }
    }
    
    uint32_t reaction_ticks = pit_stop_timer();
    uint32_t reaction_ms = (reaction_ticks * 1000) / pit_get_frequency();
    
    vga_println("");
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Your reaction time: ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_print_int(reaction_ms);
    vga_println(" ms");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Rating: ");
    if (reaction_ms < 200) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        print_with_scroll("Excellent!");
    } else if (reaction_ms < 300) {
        vga_set_color(VGA_CYAN, VGA_BLCK);
        print_with_scroll("Good!");
    } else if (reaction_ms < 400) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        print_with_scroll("Average");
    } else {
        vga_set_color(VGA_RED, VGA_BLCK);
        print_with_scroll("Needs improvement");
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}
