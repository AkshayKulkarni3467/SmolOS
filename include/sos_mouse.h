#ifndef INCLUDE_SMOLOS_MOUSE_H
#define INCLUDE_SMOLOS_MOUSE_H

#include "sos_stdint.h"

#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_RIGHT_BUTTON  0x02
#define MOUSE_MIDDLE_BUTTON 0x04

typedef struct {
    int8_t x_movement;     
    int8_t y_movement;      
    uint8_t buttons;        
    int8_t z_movement;      
} MousePacket;

typedef struct {
    int x;                  
    int y;                  
    uint8_t buttons;        
    int visible;            
    int initialized;       
} MouseState;

typedef enum {
    MOUSE_EVENT_MOVE,
    MOUSE_EVENT_BUTTON_PRESS,
    MOUSE_EVENT_BUTTON_RELEASE,
    MOUSE_EVENT_SCROLL
} MouseEventType;

typedef struct {
    MouseEventType type;
    int x;                  
    int y;                  
    uint8_t buttons;        
    int8_t scroll_delta;    
} MouseEvent;

void mouse_init(void);

void mouse_get_state(MouseState* state);

void mouse_get_position(int* x, int* y);

void mouse_set_position(int x, int y);

int mouse_is_left_pressed(void);
int mouse_is_right_pressed(void);
int mouse_is_middle_pressed(void);

int mouse_get_event(MouseEvent* event);

void mouse_wait_event(MouseEvent* event);

int mouse_has_event(void);

void mouse_clear_events(void);

void mouse_show_cursor(void);
void mouse_hide_cursor(void);
void mouse_set_cursor_char(char c);
void mouse_update_cursor(void);

void mouse_set_sensitivity(int sensitivity);

void mouse_enable(void);
void mouse_disable(void);

void mouse_irq_handler(void);

uint32_t mouse_get_interrupt_count(void);

#endif // INCLUDE_SMOLOS_MOUSE_H