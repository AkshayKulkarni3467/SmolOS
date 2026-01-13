#include "sos_pit.h"
#include "sos_io.h"
#include "sos_idt.h"

static volatile uint64_t pit_ticks = 0;
static uint32_t pit_frequency = PIT_FREQ_1000HZ;
static timer_callback_t user_callback = 0;
static volatile uint64_t performance_timer_start = 0;

void pit_init(uint32_t frequency) {
    pit_frequency = frequency;
    
    uint32_t divisor = PIT_BASE_FREQ / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;
    
    outb(PIT_COMMAND, 0x36);
    
    outb(PIT_CHANNEL_0, divisor & 0xFF);
    outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF);
    
    pit_ticks = 0;
    
    irq_install_handler(0, pit_irq_handler);
}

void pit_set_frequency(uint32_t frequency) {
    pit_init(frequency);
}

uint32_t pit_get_frequency(void) {
    return pit_frequency;
}

void pit_irq_handler(void) {
    pit_ticks++;
    
    if (user_callback) {
        user_callback();
    }
}

void pit_wait(uint32_t ticks) {
    uint64_t target = pit_ticks + ticks;
    while (pit_ticks < target) {
        __asm__ volatile("hlt");  
    }
}

void pit_delay_ms(uint32_t milliseconds) {
    uint32_t ticks = (milliseconds * pit_frequency) / 1000;
    if (ticks == 0) ticks = 1;  
    pit_wait(ticks);
}

void pit_delay_us(uint32_t microseconds) {
    uint64_t ticks = ((uint64_t)microseconds * pit_frequency) / 1000000;
    if (ticks == 0) {
        for (volatile uint32_t i = 0; i < microseconds; i++) {
            __asm__ volatile("nop");
        }
    } else {
        pit_wait(ticks);
    }
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

uint32_t pit_get_seconds(void) {
    return (uint32_t)(pit_ticks / pit_frequency);
}

uint32_t pit_get_milliseconds(void) {
    uint64_t total_ms = (pit_ticks * 1000) / pit_frequency;
    return (uint32_t)(total_ms % 1000);
}

uint32_t pit_get_total_milliseconds(void) {
    uint64_t total_ms = (pit_ticks * 1000) / pit_frequency;
    return (uint32_t)total_ms;
}

void pit_format_uptime(char* buffer) {
    uint32_t total_seconds = pit_get_seconds();
    
    uint32_t days = total_seconds / 86400;
    total_seconds %= 86400;
    uint32_t hours = total_seconds / 3600;
    total_seconds %= 3600;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    
    int pos = 0;
    
    if (days > 0) {
        if (days >= 10) buffer[pos++] = '0' + (days / 10);
        buffer[pos++] = '0' + (days % 10);
        buffer[pos++] = 'd';
        buffer[pos++] = ' ';
    }
    
    if (hours > 0 || days > 0) {
        if (hours >= 10) buffer[pos++] = '0' + (hours / 10);
        buffer[pos++] = '0' + (hours % 10);
        buffer[pos++] = 'h';
        buffer[pos++] = ' ';
    }
    
    if (minutes >= 10) buffer[pos++] = '0' + (minutes / 10);
    buffer[pos++] = '0' + (minutes % 10);
    buffer[pos++] = 'm';
    buffer[pos++] = ' ';
    
    if (seconds >= 10) buffer[pos++] = '0' + (seconds / 10);
    buffer[pos++] = '0' + (seconds % 10);
    buffer[pos++] = 's';
    
    buffer[pos] = '\0';
}

void pit_register_callback(timer_callback_t callback) {
    user_callback = callback;
}

void pit_unregister_callback(void) {
    user_callback = 0;
}

void pit_start_timer(void) {
    performance_timer_start = pit_ticks;
}

uint32_t pit_stop_timer(void) {
    return (uint32_t)(pit_ticks - performance_timer_start);
}

uint64_t pit_get_interrupt_count(void) {
    return pit_ticks;
}

void delay(uint32_t ms) {
    pit_delay_ms(ms);
}