#ifndef INCLUDE_SMOLOS_RTC_H
#define INCLUDE_SMOLOS_RTC_H

#include "sos_stdint.h"

#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_WEEKDAY     0x06
#define RTC_DAY         0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_CENTURY     0x32

#define RTC_STATUS_A    0x0A
#define RTC_STATUS_B    0x0B
#define RTC_STATUS_C    0x0C
#define RTC_STATUS_D    0x0D

#define RTC_ALARM_SECONDS 0x01
#define RTC_ALARM_MINUTES 0x03
#define RTC_ALARM_HOURS   0x05

#define RTC_UIP         0x80

#define RTC_SET         0x80
#define RTC_PIE         0x40
#define RTC_AIE         0x20
#define RTC_UIE         0x10
#define RTC_SQWE        0x08
#define RTC_DM          0x04
#define RTC_24_12       0x02
#define RTC_DSE         0x01

typedef struct {
    int seconds;
    int minutes;
    int hours;
    int day;
    int month;
    int year;
    int weekday;
    int is_24hour;
    int is_pm;
} RTCTime;

void rtc_init(void);

void rtc_get_time(int* hours, int* minutes, int* seconds);
void rtc_get_date(int* day, int* month, int* year);
void rtc_get_full_time(RTCTime* time);

uint8_t rtc_read(uint8_t reg);
void rtc_write(uint8_t reg, uint8_t value);

int rtc_is_updating(void);
int rtc_is_bcd_mode(void);
int rtc_is_24hour_mode(void);
uint8_t rtc_bcd_to_bin(uint8_t bcd);
uint8_t rtc_bin_to_bcd(uint8_t bin);

void rtc_set_alarm(int hours, int minutes, int seconds);
void rtc_enable_alarm(void);
void rtc_disable_alarm(void);
int rtc_alarm_triggered(void);
void rtc_clear_alarm(void);

void rtc_format_time(RTCTime* time, char* buffer);
void rtc_format_date(RTCTime* time, char* buffer);
void rtc_format_datetime(RTCTime* time, char* buffer);
const char* rtc_get_weekday_name(int weekday);
const char* rtc_get_month_name(int month);

void rtc_start_uptime(void);
uint32_t rtc_get_uptime_seconds(void);
void rtc_format_uptime(char* buffer);

void rtc_set_timezone_offset(int hours, int minutes);
void rtc_get_timezone_offset(int* hours, int* minutes);
void rtc_apply_timezone(RTCTime* time);
void rtc_get_local_time(RTCTime* time);

uint32_t rtc_get_interrupt_count(void);

void shutdown(void);
void reboot(void);

#define TZ_UTC      0, 0
#define TZ_IST      5, 30
#define TZ_EST     -5, 0
#define TZ_PST     -8, 0
#define TZ_CST      8, 0
#define TZ_JST      9, 0
#define TZ_AEST    10, 0

#endif //INCLUDE_SMOLOS_RTC_H