#include "sos_rtc.h"
#include "sos_io.h"
#include "sos_string.h"
#include "sos_idt.h"
#include "sos_vga.h"  

#define RTC_ADDRESS 0x70
#define RTC_DATA    0x71

#define NMI_DISABLE 0x80

static int rtc_initialized = 0;
static uint32_t uptime_seconds = 0;
static int uptime_tracking = 0;
static int timezone_hours = 5;
static int timezone_minutes = 30;
static volatile int rtc_tick_count = 0;
static volatile int alarm_triggered = 0;
static volatile int rtc_interrupt_count = 0;  

void rtc_irq_handler(void);

static void rtc_wait_for_update(void) {
    int timeout = 10000;
    while (rtc_is_updating() && timeout > 0) {
        timeout--;
    }
}

int rtc_is_updating(void) {
    outb(RTC_ADDRESS, NMI_DISABLE | RTC_STATUS_A);
    return (inb(RTC_DATA) & RTC_UIP) != 0;
}

uint8_t rtc_read(uint8_t reg) {
    outb(RTC_ADDRESS, NMI_DISABLE | reg);
    return inb(RTC_DATA);
}

void rtc_write(uint8_t reg, uint8_t value) {
    outb(RTC_ADDRESS, NMI_DISABLE | reg);
    outb(RTC_DATA, value);
}

int rtc_is_bcd_mode(void) {
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    return !(status_b & RTC_DM);
}

int rtc_is_24hour_mode(void) {
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    return (status_b & RTC_24_12) != 0;
}

uint8_t rtc_bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

uint8_t rtc_bin_to_bcd(uint8_t bin) {
    return ((bin / 10) << 4) | (bin % 10);
}

void rtc_init(void) {
    if (rtc_initialized) {
        return;
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    vga_println("[RTC] Reading Status B...");
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    vga_print("[RTC] Current Status B: 0x");
    vga_print_hex(status_b);
    vga_println("");
    
    vga_println("[RTC] Disabling updates...");
    status_b |= RTC_SET;
    rtc_write(RTC_STATUS_B, status_b);
    
    vga_println("[RTC] Setting 24-hour and binary mode...");
    status_b |= RTC_24_12 | RTC_DM;
    
    vga_println("[RTC] Enabling periodic interrupt...");
    status_b |= RTC_PIE;
    
    vga_println("[RTC] Re-enabling updates...");
    status_b &= ~RTC_SET;
    rtc_write(RTC_STATUS_B, status_b);
    
    status_b = rtc_read(RTC_STATUS_B);
    vga_print("[RTC] Final Status B: 0x");
    vga_print_hex(status_b);
    vga_println("");
    
    vga_println("[RTC] Setting interrupt rate to 1024 Hz...");
    uint8_t status_a = rtc_read(RTC_STATUS_A);
    vga_print("[RTC] Current Status A: 0x");
    vga_print_hex(status_a);
    vga_println("");
    
    status_a = (status_a & 0xF0) | 0x06;
    rtc_write(RTC_STATUS_A, status_a);
    
    status_a = rtc_read(RTC_STATUS_A);
    vga_print("[RTC] Final Status A: 0x");
    vga_print_hex(status_a);
    vga_println("");
    
    vga_println("[RTC] Clearing pending interrupts...");
    uint8_t status_c = rtc_read(RTC_STATUS_C);
    vga_print("[RTC] Status C: 0x");
    vga_print_hex(status_c);
    vga_println("");
    
    vga_println("[RTC] Installing IRQ8 handler...");
    irq_install_handler(8, rtc_irq_handler);
    vga_println("[RTC] Handler installed");
    
    vga_println("[RTC] Unmasking IRQ8 in PIC...");
    uint8_t mask = inb(0xA1);
    vga_print("[RTC] Current PIC2 mask: 0x");
    vga_print_hex(mask);
    vga_println("");
    
    mask &= ~0x01;
    outb(0xA1, mask);
    
    mask = inb(0xA1);
    vga_print("[RTC] New PIC2 mask: 0x");
    vga_print_hex(mask);
    vga_println("");
    
    if (mask & 0x01) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_println("[RTC] ERROR: IRQ8 still masked!");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    } else {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_println("[RTC] SUCCESS: IRQ8 unmasked");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    }
    
    vga_println("[RTC] Waiting for first interrupt...");
    uint32_t old_count = rtc_interrupt_count;
    
    for (volatile int i = 0; i < 2000000; i++) {
        if (rtc_interrupt_count > old_count) {
            vga_set_color(VGA_LGREEN, VGA_BLCK);
            vga_println("[RTC] *** INTERRUPT RECEIVED! ***");
            vga_print("[RTC] Interrupt count: ");
            vga_print_int(rtc_interrupt_count);
            vga_println("");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            break;
        }
    }
    
    if (rtc_interrupt_count == old_count) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_println("[RTC] *** NO INTERRUPT RECEIVED ***");
        vga_println("[RTC] Check:");
        vga_println("[RTC]   1. IDT initialized?");
        vga_println("[RTC]   2. Interrupts enabled globally?");
        vga_println("[RTC]   3. Assembly stubs correct?");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    }
    
    rtc_initialized = 1;
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("[RTC] Initialization complete");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("");
}

void rtc_get_time(int* hours, int* minutes, int* seconds) {
    if (!rtc_initialized) rtc_init();
    
    rtc_wait_for_update();
    
    uint8_t seconds_val = rtc_read(RTC_SECONDS);
    uint8_t minutes_val = rtc_read(RTC_MINUTES);
    uint8_t hours_val = rtc_read(RTC_HOURS);
    
    if (rtc_is_bcd_mode()) {
        *seconds = rtc_bcd_to_bin(seconds_val);
        *minutes = rtc_bcd_to_bin(minutes_val);
        *hours = rtc_bcd_to_bin(hours_val & 0x7F);
    } else {
        *seconds = seconds_val;
        *minutes = minutes_val;
        *hours = hours_val & 0x7F;
    }
    
    if (!rtc_is_24hour_mode()) {
        int is_pm = (hours_val & 0x80) != 0;
        if (is_pm && *hours != 12) {
            *hours += 12;
        } else if (!is_pm && *hours == 12) {
            *hours = 0;
        }
    }
}

void rtc_get_date(int* day, int* month, int* year) {
    if (!rtc_initialized) rtc_init();
    
    rtc_wait_for_update();
    
    uint8_t day_val = rtc_read(RTC_DAY);
    uint8_t month_val = rtc_read(RTC_MONTH);
    uint8_t year_val = rtc_read(RTC_YEAR);
    
    if (rtc_is_bcd_mode()) {
        *day = rtc_bcd_to_bin(day_val);
        *month = rtc_bcd_to_bin(month_val);
        *year = rtc_bcd_to_bin(year_val);
    } else {
        *day = day_val;
        *month = month_val;
        *year = year_val;
    }
    
    *year += 2000;
}

void rtc_get_full_time(RTCTime* time) {
    if (!rtc_initialized) rtc_init();
    
    rtc_wait_for_update();
    
    uint8_t seconds_val = rtc_read(RTC_SECONDS);
    uint8_t minutes_val = rtc_read(RTC_MINUTES);
    uint8_t hours_val = rtc_read(RTC_HOURS);
    uint8_t weekday_val = rtc_read(RTC_WEEKDAY);
    uint8_t day_val = rtc_read(RTC_DAY);
    uint8_t month_val = rtc_read(RTC_MONTH);
    uint8_t year_val = rtc_read(RTC_YEAR);
    
    int is_bcd = rtc_is_bcd_mode();
    
    if (is_bcd) {
        time->seconds = rtc_bcd_to_bin(seconds_val);
        time->minutes = rtc_bcd_to_bin(minutes_val);
        time->hours = rtc_bcd_to_bin(hours_val & 0x7F);
        time->weekday = rtc_bcd_to_bin(weekday_val);
        time->day = rtc_bcd_to_bin(day_val);
        time->month = rtc_bcd_to_bin(month_val);
        time->year = rtc_bcd_to_bin(year_val);
    } else {
        time->seconds = seconds_val;
        time->minutes = minutes_val;
        time->hours = hours_val & 0x7F;
        time->weekday = weekday_val;
        time->day = day_val;
        time->month = month_val;
        time->year = year_val;
    }
    
    time->year += 2000;
    time->is_24hour = rtc_is_24hour_mode();
    
    if (!time->is_24hour) {
        time->is_pm = (hours_val & 0x80) != 0;
        if (time->is_pm && time->hours != 12) {
            time->hours += 12;
        } else if (!time->is_pm && time->hours == 12) {
            time->hours = 0;
        }
    } else {
        time->is_pm = time->hours >= 12;
    }
}

void rtc_set_alarm(int hours, int minutes, int seconds) {
    if (!rtc_initialized) rtc_init();
    
    minutes -= timezone_minutes;
    hours -= timezone_hours;
    
    if (minutes < 0) {
        minutes += 60;
        hours--;
    }
    if (hours < 0) {
        hours += 24;
    }
    
    uint8_t sec_val = rtc_is_bcd_mode() ? rtc_bin_to_bcd(seconds) : seconds;
    uint8_t min_val = rtc_is_bcd_mode() ? rtc_bin_to_bcd(minutes) : minutes;
    uint8_t hour_val = rtc_is_bcd_mode() ? rtc_bin_to_bcd(hours) : hours;
    
    rtc_write(RTC_ALARM_SECONDS, sec_val);
    rtc_write(RTC_ALARM_MINUTES, min_val);
    rtc_write(RTC_ALARM_HOURS, hour_val);
}

void rtc_enable_alarm(void) {
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    status_b |= RTC_AIE;
    rtc_write(RTC_STATUS_B, status_b);
    
    rtc_read(RTC_STATUS_C);
}

void rtc_disable_alarm(void) {
    uint8_t status_b = rtc_read(RTC_STATUS_B);
    status_b &= ~RTC_AIE;
    rtc_write(RTC_STATUS_B, status_b);
}

void rtc_set_timezone_offset(int hours, int minutes) {
    timezone_hours = hours;
    timezone_minutes = minutes;
}

void rtc_get_timezone_offset(int* hours, int* minutes) {
    *hours = timezone_hours;
    *minutes = timezone_minutes;
}

void rtc_apply_timezone(RTCTime* time) {
    time->minutes += timezone_minutes;
    time->hours += timezone_hours;
    
    if (time->minutes >= 60) {
        time->minutes -= 60;
        time->hours++;
    } else if (time->minutes < 0) {
        time->minutes += 60;
        time->hours--;
    }
    
    if (time->hours >= 24) {
        time->hours -= 24;
        time->day++;
        
        int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (time->month == 2 && time->year % 4 == 0 && 
            (time->year % 100 != 0 || time->year % 400 == 0)) {
            days_in_month[2] = 29;
        }
        
        if (time->day > days_in_month[time->month]) {
            time->day = 1;
            time->month++;
            if (time->month > 12) {
                time->month = 1;
                time->year++;
            }
        }
        
        time->weekday++;
        if (time->weekday > 7) time->weekday = 1;
        
    } else if (time->hours < 0) {
        time->hours += 24;
        time->day--;
        
        if (time->day < 1) {
            time->month--;
            if (time->month < 1) {
                time->month = 12;
                time->year--;
            }
            
            int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (time->month == 2 && time->year % 4 == 0 && 
                (time->year % 100 != 0 || time->year % 400 == 0)) {
                days_in_month[2] = 29;
            }
            time->day = days_in_month[time->month];
        }
        
        time->weekday--;
        if (time->weekday < 1) time->weekday = 7;
    }
}

void rtc_get_local_time(RTCTime* time) {
    rtc_get_full_time(time);
    rtc_apply_timezone(time);
}

const char* rtc_get_weekday_name(int weekday) {
    static const char* names[] = {
        "Unknown", "Sunday", "Monday", "Tuesday", 
        "Wednesday", "Thursday", "Friday", "Saturday"
    };
    if (weekday < 1 || weekday > 7) return names[0];
    return names[weekday];
}

const char* rtc_get_month_name(int month) {
    static const char* names[] = {
        "Unknown", "January", "February", "March", "April",
        "May", "June", "July", "August", "September",
        "October", "November", "December"
    };
    if (month < 1 || month > 12) return names[0];
    return names[month];
}

void rtc_format_time(RTCTime* time, char* buffer) {
    int pos = 0;
    buffer[pos++] = '0' + (time->hours / 10);
    buffer[pos++] = '0' + (time->hours % 10);
    buffer[pos++] = ':';
    buffer[pos++] = '0' + (time->minutes / 10);
    buffer[pos++] = '0' + (time->minutes % 10);
    buffer[pos++] = ':';
    buffer[pos++] = '0' + (time->seconds / 10);
    buffer[pos++] = '0' + (time->seconds % 10);
    buffer[pos] = '\0';
}

void rtc_format_date(RTCTime* time, char* buffer) {
    int pos = 0;
    buffer[pos++] = '0' + (time->day / 10);
    buffer[pos++] = '0' + (time->day % 10);
    buffer[pos++] = '/';
    buffer[pos++] = '0' + (time->month / 10);
    buffer[pos++] = '0' + (time->month % 10);
    buffer[pos++] = '/';
    buffer[pos++] = '0' + (time->year / 1000);
    buffer[pos++] = '0' + ((time->year / 100) % 10);
    buffer[pos++] = '0' + ((time->year / 10) % 10);
    buffer[pos++] = '0' + (time->year % 10);
    buffer[pos] = '\0';
}

void rtc_format_datetime(RTCTime* time, char* buffer) {
    char date_buf[16];
    char time_buf[16];
    rtc_format_date(time, date_buf);
    rtc_format_time(time, time_buf);
    strcpy(buffer, date_buf);
    strcat(buffer, " ");
    strcat(buffer, time_buf);
}

void rtc_start_uptime(void) {
    uptime_seconds = 0;
    uptime_tracking = 1;
    rtc_tick_count = 0;
}

uint32_t rtc_get_uptime_seconds(void) {
    return uptime_seconds;
}

void rtc_format_uptime(char* buffer) {
    uint32_t seconds = uptime_seconds;
    uint32_t days = seconds / 86400;
    seconds %= 86400;
    uint32_t hours = seconds / 3600;
    seconds %= 3600;
    uint32_t minutes = seconds / 60;
    
    int pos = 0;
    
    if (days > 0) {
        if (days >= 10) buffer[pos++] = '0' + (days / 10);
        buffer[pos++] = '0' + (days % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'd';
        buffer[pos++] = 'a';
        buffer[pos++] = 'y';
        if (days != 1) buffer[pos++] = 's';
        buffer[pos++] = ',';
        buffer[pos++] = ' ';
    }
    
    if (hours > 0 || days > 0) {
        if (hours >= 10) buffer[pos++] = '0' + (hours / 10);
        buffer[pos++] = '0' + (hours % 10);
        buffer[pos++] = ' ';
        buffer[pos++] = 'h';
        buffer[pos++] = 'r';
        if (hours != 1) buffer[pos++] = 's';
        buffer[pos++] = ',';
        buffer[pos++] = ' ';
    }
    
    if (minutes >= 10) buffer[pos++] = '0' + (minutes / 10);
    buffer[pos++] = '0' + (minutes % 10);
    buffer[pos++] = ' ';
    buffer[pos++] = 'm';
    buffer[pos++] = 'i';
    buffer[pos++] = 'n';
    
    buffer[pos] = '\0';
}

int rtc_alarm_triggered(void) {
    return alarm_triggered;
}

void rtc_clear_alarm(void) {
    alarm_triggered = 0;
}

uint32_t rtc_get_interrupt_count(void) {
    return rtc_interrupt_count;
}

void rtc_irq_handler(void) {
    rtc_interrupt_count++;
    
    uint8_t status_c = rtc_read(RTC_STATUS_C);
    
    if (status_c & 0x40) { 
        rtc_tick_count++;
        
        if (rtc_tick_count >= 1024) {
            rtc_tick_count = 0;
            if (uptime_tracking) {
                uptime_seconds++;
            }
        }
    }
    
    if (status_c & 0x20) {  
        alarm_triggered = 1;
    }
}

static int get_rtc_seconds(void) {
    int hours, minutes, seconds;
    rtc_get_time(&hours, &minutes, &seconds);
    return seconds;
}

static void wait_seconds(int seconds) {
    int start_second = get_rtc_seconds();
    int target_second = (start_second + seconds) % 60;
    
    while (get_rtc_seconds() != target_second) {
        for (volatile int i = 0; i < 1000; i++);
    }
}

void shutdown(void) {
    vga_clear();
    vga_println("=== SYSTEM SHUTDOWN ===");
    vga_println("Goodbye from SmolOS!");
    vga_println("Shutting down in 3 seconds...");
    
    vga_print(" 3...");
    wait_seconds(1);
    
    vga_print(" 2...");
    wait_seconds(1);
    
    vga_println(" 1...");
    wait_seconds(1);
    
    vga_println("Shutting down NOW!");
    
    for (volatile int i = 0; i < 1000000; i++);
    
    
    asm volatile ("outw %0, %1" : : "a"((short)0x2000), "Nd"((short)0x604));
    
    asm volatile ("outw %0, %1" : : "a"((short)0x2000), "Nd"((short)0xB004));
    
    asm volatile ("outw %0, %1" : : "a"((short)0x3400), "Nd"((short)0x4004));
    
    vga_println("Shutdown failed. Halting CPU...");
    asm volatile ("hlt");
    
    while (1) {
        asm volatile ("hlt");
    }
}

void reboot(void) {
    vga_clear();
    vga_println("=== SYSTEM REBOOT ===");
    vga_println("Restarting SmolOS in 3 seconds...");
    
    vga_print("3...");
    wait_seconds(1);
    
    vga_print(" 2...");
    wait_seconds(1);
    
    vga_println(" 1...");
    wait_seconds(1);
    
    vga_println("Rebooting NOW!");
    
    for (volatile int i = 0; i < 1000000; i++);
    
    asm volatile ("cli");
    
    unsigned char good;
    do {
        asm volatile ("inb $0x64, %0" : "=a"(good));
    } while (good & 0x02);
    
    asm volatile ("outb %0, $0x64" : : "a"((unsigned char)0xFE));
    
    asm volatile ("ud2");
    
    while (1) {
        asm volatile ("hlt");
    }
}
