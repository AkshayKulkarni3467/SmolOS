#ifndef INCLUDE_SMOLOS_KEYBOARD_H
#define INCLUDE_SMOLOS_KEYBOARD_H

#include "sos_stdint.h"

/* KEY CODES */
typedef enum {
    KEY_UNKNOWN = 0,
    KEY_CHAR,           
    

    KEY_F1_KEY,
    KEY_F2_KEY,
    KEY_F3_KEY,
    KEY_F4_KEY,
    KEY_F5_KEY,
    KEY_F6_KEY,
    KEY_F7_KEY,
    KEY_F8_KEY,
    KEY_F9_KEY,
    KEY_F10_KEY,
    KEY_F11_KEY,
    KEY_F12_KEY,
    

    KEY_ARROW_UP,
    KEY_ARROW_DOWN,
    KEY_ARROW_LEFT,
    KEY_ARROW_RIGHT,
    

    KEY_HOME_KEY,
    KEY_END_KEY,
    KEY_PAGEUP_KEY,
    KEY_PAGEDOWN_KEY,
    KEY_INSERT_KEY,
    KEY_DELETE_KEY,
    

    KEY_ESCAPE,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_ENTER,
    KEY_SPACE
} KeyCode;


typedef struct {
    KeyCode key_code;      
    char character;       
    int shift;             
    int ctrl;              
    int alt;               
} KeyEvent;


#define CHAR_F1     0x01
#define CHAR_F2     0x02
#define CHAR_F3     0x03
#define CHAR_F4     0x04
#define CHAR_F5     0x05
#define CHAR_F6     0x06
#define CHAR_F7     0x07
#define CHAR_F8     0x08
#define CHAR_F9     0x09
#define CHAR_F10    0x0A
#define CHAR_F11    0x0B
#define CHAR_F12    0x0C

#define CHAR_UP     0x11
#define CHAR_DOWN   0x12
#define CHAR_LEFT   0x13
#define CHAR_RIGHT  0x14
#define CHAR_HOME   0x15
#define CHAR_END    0x16
#define CHAR_DELETE 0x17
#define CHAR_INSERT 0x18
#define CHAR_PAGEUP 0x19
#define CHAR_PAGEDOWN 0x1A



void keyboard_init(void);
void keyboard_poll(void);

char get_char(void);
char wait_for_char(void);

int has_key(void);
void clear_key_buffer(void);

int is_shift_pressed(void);
int is_ctrl_pressed(void);
int is_alt_pressed(void);

int is_capslock_on(void);
int is_numlock_on(void);
int is_scrolllock_on(void);


int read_line(char* buffer, int max_length);
KeyEvent get_key_event(void);
int has_key_event(void);
KeyEvent wait_for_key_event(void);
void clear_event_buffer(void);

int is_key_combo(KeyEvent event, KeyCode key, int ctrl, int shift, int alt);

int check_ctrl_key(char key);

int is_printable(char c);
int is_digit(char c);
int is_alpha(char c);
int is_whitespace(char c);
int is_special_key(char c);

void keyboard_set_leds(int caps, int num, int scroll);
void keyboard_update_leds(void);

int is_up_pressed(void);
int is_left_pressed(void);
int is_right_pressed(void);
int is_down_pressed(void);

#endif // INCLUDE_SMOLOS_KEYBOARD_H