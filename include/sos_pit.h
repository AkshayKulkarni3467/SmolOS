#ifndef INCLUDE_SMOLOS_PIT_H
#define INCLUDE_SMOLOS_PIT_H

#include "sos_stdint.h"

#define PIT_CHANNEL_0   0x40
#define PIT_CHANNEL_1   0x41
#define PIT_CHANNEL_2   0x42
#define PIT_COMMAND     0x43

#define PIT_BASE_FREQ   1193182

#define PIT_FREQ_1000HZ 1000  
#define PIT_FREQ_100HZ  100   
#define PIT_FREQ_50HZ   50    
#define PIT_FREQ_18HZ   18    

typedef void (*timer_callback_t)(void);

void pit_init(uint32_t frequency);
void pit_set_frequency(uint32_t frequency);
uint32_t pit_get_frequency(void);

void pit_wait(uint32_t ticks);
void pit_delay_ms(uint32_t milliseconds);
void pit_delay_us(uint32_t microseconds);

uint64_t pit_get_ticks(void);
uint32_t pit_get_seconds(void);
uint32_t pit_get_milliseconds(void);
void pit_format_uptime(char* buffer);

void pit_register_callback(timer_callback_t callback);
void pit_unregister_callback(void);

void pit_start_timer(void);
uint32_t pit_stop_timer(void);  

void pit_irq_handler(void);

uint64_t pit_get_interrupt_count(void);

#endif // INCLUDE_SMOLOS_PIT_H