#include "sos_pit.h"

void delay(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        uint16_t count = 1193;  // 1193182 Hz / 1193 = ~1000 Hz = 1ms
        
        outb(PIT_CMD, 0xB0);  
        outb(PIT_CH2, count & 0xFF);
        outb(PIT_CH2, (count >> 8) & 0xFF);
        
        uint16_t current;
        do {
            outb(PIT_CMD, 0x80);  
            uint8_t low = inb(PIT_CH2);
            uint8_t high = inb(PIT_CH2);
            current = (high << 8) | low;
        } while (current > 10); 
    }
}