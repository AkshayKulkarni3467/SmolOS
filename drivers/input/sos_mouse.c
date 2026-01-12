#include "sos_mouse.h"
#include "sos_io.h"
#include "sos_idt.h"
#include "sos_vga.h"

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

#define PS2_CMD_READ_CONFIG     0x20
#define PS2_CMD_WRITE_CONFIG    0x60
#define PS2_CMD_DISABLE_PORT2   0xA7
#define PS2_CMD_ENABLE_PORT2    0xA8
#define PS2_CMD_WRITE_PORT2     0xD4

#define MOUSE_CMD_RESET         0xFF
#define MOUSE_CMD_ENABLE        0xF4
#define MOUSE_CMD_DISABLE       0xF5
#define MOUSE_CMD_SET_DEFAULTS  0xF6
#define MOUSE_CMD_SET_SAMPLE    0xF3
#define MOUSE_CMD_GET_ID        0xF2

#define MOUSE_ACK               0xFA
#define MOUSE_RESEND            0xFE

#define EVENT_BUFFER_SIZE 32

static MouseState mouse_state;
static MouseEvent event_buffer[EVENT_BUFFER_SIZE];
static volatile uint8_t event_head = 0;
static volatile uint8_t event_tail = 0;

static volatile uint8_t mouse_cycle = 0;
static volatile uint8_t mouse_packet[4];
static volatile uint32_t mouse_interrupt_count = 0;

static char cursor_char = 0xDB;  
static uint8_t saved_char = ' ';
static uint8_t saved_color = 0x07;
static int sensitivity = 2;  

static int accumulated_x = 0;
static int accumulated_y = 0;

static void mouse_wait_input(void) {
    int timeout = 100000;
    while (timeout--) {
        if (inb(PS2_STATUS_PORT) & 0x01) {
            return;
        }
    }
}

static void mouse_wait_output(void) {
    int timeout = 100000;
    while (timeout--) {
        if (!(inb(PS2_STATUS_PORT) & 0x02)) {
            return;
        }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_PORT2);
    mouse_wait_output();
    outb(PS2_DATA_PORT, data);
}

static uint8_t mouse_read(void) {
    mouse_wait_input();
    return inb(PS2_DATA_PORT);
}

static void mouse_add_event(MouseEvent* event) {
    event_buffer[event_head] = *event;
    event_head = (event_head + 1) % EVENT_BUFFER_SIZE;
    
    if (event_head == event_tail) {
        event_tail = (event_tail + 1) % EVENT_BUFFER_SIZE;
    }
}

static void mouse_save_under_cursor(void) {
    if (mouse_state.x >= 0 && mouse_state.x < 80 && 
        mouse_state.y >= 0 && mouse_state.y < 25) {
        size_t index = mouse_state.y * 80 + mouse_state.x;
        uint16_t entry = VGA_MEM[index];
        saved_char = entry & 0xFF;
        saved_color = (entry >> 8) & 0xFF;
    }
}


static void mouse_restore_under_cursor(void) {
    if (mouse_state.x >= 0 && mouse_state.x < 80 && 
        mouse_state.y >= 0 && mouse_state.y < 25) {
        vga_putchr_direct(mouse_state.x, mouse_state.y, saved_char, saved_color);
    }
}

void mouse_init(void) {
    mouse_state.x = 40;
    mouse_state.y = 12;
    mouse_state.buttons = 0;
    mouse_state.visible = 1;
    mouse_state.initialized = 0;
    
    mouse_cycle = 0;
    event_head = 0;
    event_tail = 0;
    mouse_interrupt_count = 0;
    accumulated_x = 0;
    accumulated_y = 0;
    
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_ENABLE_PORT2);
    
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_READ_CONFIG);
    mouse_wait_input();
    uint8_t config = inb(PS2_DATA_PORT);
    config |= 0x02;  
    config &= ~0x20; 
    mouse_wait_output();
    outb(PS2_COMMAND_PORT, PS2_CMD_WRITE_CONFIG);
    mouse_wait_output();
    outb(PS2_DATA_PORT, config);
    
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    mouse_read(); 
    mouse_write(MOUSE_CMD_SET_SAMPLE);
    mouse_read();
    mouse_write(100);
    mouse_read();
    
    mouse_write(MOUSE_CMD_ENABLE);
    mouse_read();
    
    irq_install_handler(12, mouse_irq_handler);
    
    uint8_t mask = inb(0xA1);
    mask &= ~0x10;
    outb(0xA1, mask);
    
    mouse_state.initialized = 1;
    mouse_save_under_cursor();
}

void mouse_irq_handler(void) {
    mouse_interrupt_count++;
    
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & 0x20)) {
        return;
    }
    
    uint8_t data = inb(PS2_DATA_PORT);
    
    mouse_packet[mouse_cycle] = data;
    mouse_cycle++;
    
    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        
        uint8_t flags = mouse_packet[0];
        uint8_t x_raw = mouse_packet[1];
        uint8_t y_raw = mouse_packet[2];
        

        if (!(flags & 0x08)) {
            return;
        }
        

        int16_t x_delta = x_raw;
        int16_t y_delta = y_raw;
        

        if (flags & 0x10) {

            x_delta = (int16_t)((int8_t)x_raw);
        }
        if (flags & 0x20) {

            y_delta = (int16_t)((int8_t)y_raw);
        }
        

        y_delta = -y_delta;
        

        accumulated_x += x_delta;
        accumulated_y += y_delta;

        int divisor_x = 9; 
        int divisor_y = 15; 
        

        int screen_x_delta = accumulated_x / divisor_x;
        int screen_y_delta = accumulated_y / divisor_y;
        

        accumulated_x %= divisor_x;
        accumulated_y %= divisor_y;
        
        int moved = (screen_x_delta != 0 || screen_y_delta != 0);
        
        uint8_t old_buttons = mouse_state.buttons;
        mouse_state.buttons = flags & 0x07;
        
        if (mouse_state.visible && moved) {
            mouse_restore_under_cursor();
        }
        
        if (moved) {
            mouse_state.x += screen_x_delta;
            mouse_state.y += screen_y_delta;
            
            if (mouse_state.x < 0) mouse_state.x = 0;
            if (mouse_state.x >= 80) mouse_state.x = 79;
            if (mouse_state.y < 0) mouse_state.y = 0;
            if (mouse_state.y >= 25) mouse_state.y = 24;
            
            if (mouse_state.visible) {
                mouse_save_under_cursor();
                mouse_update_cursor();
            }
            
            MouseEvent event;
            event.type = MOUSE_EVENT_MOVE;
            event.x = mouse_state.x;
            event.y = mouse_state.y;
            event.buttons = mouse_state.buttons;
            event.scroll_delta = 0;
            mouse_add_event(&event);
        }
        
        for (int i = 0; i < 3; i++) {
            uint8_t mask = 1 << i;
            int old_pressed = old_buttons & mask;
            int new_pressed = mouse_state.buttons & mask;
            
            if (!old_pressed && new_pressed) {
                MouseEvent event;
                event.type = MOUSE_EVENT_BUTTON_PRESS;
                event.x = mouse_state.x;
                event.y = mouse_state.y;
                event.buttons = mouse_state.buttons;
                event.scroll_delta = 0;
                mouse_add_event(&event);
            } else if (old_pressed && !new_pressed) {
                MouseEvent event;
                event.type = MOUSE_EVENT_BUTTON_RELEASE;
                event.x = mouse_state.x;
                event.y = mouse_state.y;
                event.buttons = mouse_state.buttons;
                event.scroll_delta = 0;
                mouse_add_event(&event);
            }
        }
    }
}

void mouse_get_state(MouseState* state) {
    *state = mouse_state;
}

void mouse_get_position(int* x, int* y) {
    *x = mouse_state.x;
    *y = mouse_state.y;
}

void mouse_set_position(int x, int y) {
    if (mouse_state.visible) {
        mouse_restore_under_cursor();
    }
    
    mouse_state.x = x;
    mouse_state.y = y;
    
    if (mouse_state.x < 0) mouse_state.x = 0;
    if (mouse_state.x >= 80) mouse_state.x = 79;
    if (mouse_state.y < 0) mouse_state.y = 0;
    if (mouse_state.y >= 25) mouse_state.y = 24;
    
    if (mouse_state.visible) {
        mouse_save_under_cursor();
        mouse_update_cursor();
    }
}

int mouse_is_left_pressed(void) {
    return (mouse_state.buttons & MOUSE_LEFT_BUTTON) != 0;
}

int mouse_is_right_pressed(void) {
    return (mouse_state.buttons & MOUSE_RIGHT_BUTTON) != 0;
}

int mouse_is_middle_pressed(void) {
    return (mouse_state.buttons & MOUSE_MIDDLE_BUTTON) != 0;
}

int mouse_get_event(MouseEvent* event) {
    if (event_head == event_tail) {
        return 0;
    }
    
    *event = event_buffer[event_tail];
    event_tail = (event_tail + 1) % EVENT_BUFFER_SIZE;
    return 1;
}

void mouse_wait_event(MouseEvent* event) {
    while (!mouse_has_event()) {
        __asm__ volatile("hlt");
    }
    mouse_get_event(event);
}

int mouse_has_event(void) {
    return event_head != event_tail;
}

void mouse_clear_events(void) {
    event_head = 0;
    event_tail = 0;
}

void mouse_show_cursor(void) {
    if (!mouse_state.visible) {
        mouse_state.visible = 1;
        mouse_save_under_cursor();
        mouse_update_cursor();
    }
}

void mouse_hide_cursor(void) {
    if (mouse_state.visible) {
        mouse_restore_under_cursor();
        mouse_state.visible = 0;
    }
}

void mouse_set_cursor_char(char c) {
    cursor_char = c;
    if (mouse_state.visible) {
        mouse_update_cursor();
    }
}

void mouse_update_cursor(void) {
    if (!mouse_state.visible) return;
    
    if (mouse_state.x >= 0 && mouse_state.x < 80 && 
        mouse_state.y >= 0 && mouse_state.y < 25) {
        vga_putchr_direct(mouse_state.x, mouse_state.y, cursor_char, 0x70);
    }
}

void mouse_set_sensitivity(int sens) {
    if (sens < 1) sens = 1;
    if (sens > 10) sens = 10;
    sensitivity = sens;
    
    accumulated_x = 0;
    accumulated_y = 0;
}

void mouse_enable(void) {
    mouse_write(MOUSE_CMD_ENABLE);
    mouse_read();
}

void mouse_disable(void) {
    mouse_write(MOUSE_CMD_DISABLE);
    mouse_read();
}

uint32_t mouse_get_interrupt_count(void) {
    return mouse_interrupt_count;
}