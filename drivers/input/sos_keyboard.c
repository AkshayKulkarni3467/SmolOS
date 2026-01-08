#include "sos_keyboard.h"
#include "sos_stdint.h"
#include "sos_io.h"

#ifdef SMOLOS_KEYBOARD_TEST
#include "sos_stdio.h"
#include <assert.h>
#endif

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_CMD_PORT    0x64
#define BUFFER_SIZE 256


static volatile char key_buffer[BUFFER_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;
static volatile int has_key_press = 0;


static volatile int shift_pressed = 0;
static volatile int ctrl_pressed = 0;
static volatile int alt_pressed = 0;
static volatile int caps_lock = 0;
static volatile int num_lock = 1; 
static volatile int scroll_lock = 0;


static volatile int extended_key = 0;


static volatile KeyEvent event_buffer[BUFFER_SIZE];
static volatile uint8_t event_head = 0;
static volatile uint8_t event_tail = 0;


#define EXTENDED_PREFIX      0xE0


#define KEY_UP               0x48
#define KEY_DOWN             0x50
#define KEY_LEFT             0x4B
#define KEY_RIGHT            0x4D
#define KEY_HOME             0x47
#define KEY_END              0x4F
#define KEY_DELETE           0x53
#define KEY_PAGEUP           0x49
#define KEY_PAGEDOWN         0x51
#define KEY_INSERT           0x52


#define KEY_LSHIFT          0x2A
#define KEY_RSHIFT          0x36
#define KEY_CAPSLOCK        0x3A
#define KEY_NUMLOCK         0x45
#define KEY_SCROLLLOCK      0x46
#define KEY_LSHIFT_RELEASE  0xAA
#define KEY_RSHIFT_RELEASE  0xB6
#define KEY_LCTRL           0x1D
#define KEY_LCTRL_RELEASE   0x9D
#define KEY_LALT            0x38
#define KEY_LALT_RELEASE    0xB8


#define KEY_F1  0x3B
#define KEY_F2  0x3C
#define KEY_F3  0x3D
#define KEY_F4  0x3E
#define KEY_F5  0x3F
#define KEY_F6  0x40
#define KEY_F7  0x41
#define KEY_F8  0x42
#define KEY_F9  0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58


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


static const char keyboard_map_lower[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',0,
    0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+',
    '1','2','3','0','.',0,0,0,0,0
};

static const char keyboard_map_upper[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b','\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',0,
    0,0,0,0,0,0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+',
    '1','2','3','0','.',0,0,0,0,0
};

void keyboard_init(void) {
    head = 0;
    tail = 0;
    event_head = 0;
    event_tail = 0;
    has_key_press = 0;
    
    shift_pressed = 0;
    ctrl_pressed = 0;
    alt_pressed = 0;
    caps_lock = 0;
    num_lock = 1;
    scroll_lock = 0;
    extended_key = 0;
    
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        inb(KEYBOARD_DATA_PORT);
    }
}

void keyboard_poll(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if (!(status & 0x01)) return;
    
    uint8_t code = inb(KEYBOARD_DATA_PORT);
    
    if (code == EXTENDED_PREFIX) {
        extended_key = 1;
        return;
    }
    
    int is_release = (code & 0x80) ? 1 : 0;
    uint8_t scancode = code & 0x7F;
    
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        shift_pressed = !is_release;
        return;
    }
    else if (scancode == KEY_LCTRL) {
        ctrl_pressed = !is_release;
        return;
    }
    else if (scancode == KEY_LALT) {
        alt_pressed = !is_release;
        return;
    }
    else if (scancode == KEY_CAPSLOCK && !is_release) {
        caps_lock = !caps_lock;
        return;
    }
    else if (scancode == KEY_NUMLOCK && !is_release) {
        num_lock = !num_lock;
        return;
    }
    else if (scancode == KEY_SCROLLLOCK && !is_release) {
        scroll_lock = !scroll_lock;
        return;
    }
    
    if (is_release) {
        extended_key = 0;
        return;
    }
    
    if (extended_key) {
        extended_key = 0;
        char special_char = 0;
        KeyCode key_code = KEY_UNKNOWN;
        
        switch(scancode) {
            case KEY_UP:      
                special_char = CHAR_UP; 
                key_code = KEY_ARROW_UP;
                break;
            case KEY_DOWN:    
                special_char = CHAR_DOWN; 
                key_code = KEY_ARROW_DOWN;
                break;
            case KEY_LEFT:    
                special_char = CHAR_LEFT; 
                key_code = KEY_ARROW_LEFT;
                break;
            case KEY_RIGHT:   
                special_char = CHAR_RIGHT; 
                key_code = KEY_ARROW_RIGHT;
                break;
            case KEY_HOME:    
                special_char = CHAR_HOME; 
                key_code = KEY_HOME_KEY;
                break;
            case KEY_END:     
                special_char = CHAR_END; 
                key_code = KEY_END_KEY;
                break;
            case KEY_DELETE:  
                special_char = CHAR_DELETE; 
                key_code = KEY_DELETE_KEY;
                break;
            case KEY_INSERT:  
                special_char = CHAR_INSERT; 
                key_code = KEY_INSERT_KEY;
                break;
            case KEY_PAGEUP:  
                special_char = CHAR_PAGEUP; 
                key_code = KEY_PAGEUP_KEY;
                break;
            case KEY_PAGEDOWN:
                special_char = CHAR_PAGEDOWN; 
                key_code = KEY_PAGEDOWN_KEY;
                break;
        }
        
        if (special_char != 0) {
            key_buffer[head] = special_char;
            head = (head + 1) % BUFFER_SIZE;
            has_key_press = 1;
            
            KeyEvent event;
            event.key_code = key_code;
            event.character = special_char;
            event.shift = shift_pressed;
            event.ctrl = ctrl_pressed;
            event.alt = alt_pressed;
            event_buffer[event_head] = event;
            event_head = (event_head + 1) % BUFFER_SIZE;
        }
        return;
    }
    
    if (scancode >= KEY_F1 && scancode <= KEY_F12) {
        char func_char = CHAR_F1 + (scancode - KEY_F1);
        KeyCode key_code = KEY_F1_KEY + (scancode - KEY_F1);
        
        key_buffer[head] = func_char;
        head = (head + 1) % BUFFER_SIZE;
        has_key_press = 1;
        
        KeyEvent event;
        event.key_code = key_code;
        event.character = func_char;
        event.shift = shift_pressed;
        event.ctrl = ctrl_pressed;
        event.alt = alt_pressed;
        event_buffer[event_head] = event;
        event_head = (event_head + 1) % BUFFER_SIZE;
        return;
    }
    
    if (scancode < 128) {
        char character = 0;
        
        int use_upper = shift_pressed;
        if (caps_lock) {
            if ((scancode >= 0x10 && scancode <= 0x1C) ||  
                (scancode >= 0x1E && scancode <= 0x26) ||  
                (scancode >= 0x2C && scancode <= 0x32)) {  
                use_upper = !use_upper;
            }
        }
        
        if (use_upper) {
            character = keyboard_map_upper[scancode];
        } else {
            character = keyboard_map_lower[scancode];
        }
        
        if (character != 0) {
            key_buffer[head] = character;
            head = (head + 1) % BUFFER_SIZE;
            has_key_press = 1;
            
            KeyEvent event;
            event.key_code = KEY_CHAR;
            event.character = character;
            event.shift = shift_pressed;
            event.ctrl = ctrl_pressed;
            event.alt = alt_pressed;
            event_buffer[event_head] = event;
            event_head = (event_head + 1) % BUFFER_SIZE;
        }
    }
}

char get_char(void) {
    if (head == tail) return 0;
    char c = key_buffer[tail];
    tail = (tail + 1) % BUFFER_SIZE;
    if (head == tail) has_key_press = 0;
    return c;
}

char wait_for_char(void) {
    while (!has_key()) {
        keyboard_poll();
        __asm__ volatile("hlt");
    }
    return get_char();
}

int has_key(void) {
    return (head != tail);
}

void clear_key_buffer(void) {
    head = 0;
    tail = 0;
    has_key_press = 0;
}

int is_shift_pressed(void) {
    return shift_pressed;
}

int is_ctrl_pressed(void) {
    return ctrl_pressed;
}

int is_alt_pressed(void) {
    return alt_pressed;
}

int is_capslock_on(void) {
    return caps_lock;
}

int is_numlock_on(void) {
    return num_lock;
}

int is_scrolllock_on(void) {
    return scroll_lock;
}

int read_line(char* buffer, int max_length) {
    int pos = 0;
    
    while (1) {
        keyboard_poll();
        
        if (!has_key()) {
            __asm__ volatile("hlt");
            continue;
        }
        
        char c = get_char();
        
        if (c == '\n') {
            buffer[pos] = '\0';
            return pos;
        }
        else if (c == '\b') {
            if (pos > 0) {
                pos--;
            }
        }
        else if (c >= 32 && c < 127) {  
            if (pos < max_length - 1) {
                buffer[pos++] = c;
            }
        }
    }
}

KeyEvent get_key_event(void) {
    KeyEvent empty_event = {KEY_UNKNOWN, 0, 0, 0, 0};
    
    if (event_head == event_tail) {
        return empty_event;
    }
    
    KeyEvent event = event_buffer[event_tail];
    event_tail = (event_tail + 1) % BUFFER_SIZE;
    return event;
}

int has_key_event(void) {
    return (event_head != event_tail);
}

KeyEvent wait_for_key_event(void) {
    while (!has_key_event()) {
        keyboard_poll();
        __asm__ volatile("hlt");
    }
    return get_key_event();
}

void clear_event_buffer(void) {
    event_head = 0;
    event_tail = 0;
}

int is_key_combo(KeyEvent event, KeyCode key, int ctrl, int shift, int alt) {
    if (event.key_code != key) return 0;
    if (ctrl && !event.ctrl) return 0;
    if (shift && !event.shift) return 0;
    if (alt && !event.alt) return 0;
    return 1;
}

int check_ctrl_key(char key) {
    KeyEvent event = get_key_event();
    if (event.ctrl && event.character == key) {
        return 1;
    }
    return 0;
}

int is_printable(char c) {
    return (c >= 32 && c < 127);
}

int is_digit(char c) {
    return (c >= '0' && c <= '9');
}

int is_alpha(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

int is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

int is_special_key(char c) {
    return (c > 0 && c < 32);
}

void keyboard_set_leds(int caps, int num, int scroll) {
    uint8_t led_byte = 0;
    if (scroll) led_byte |= 1;
    if (num)    led_byte |= 2;
    if (caps)   led_byte |= 4;
    
    while (inb(KEYBOARD_STATUS_PORT) & 0x02);
    
    outb(KEYBOARD_DATA_PORT, 0xED);
    
    while (inb(KEYBOARD_STATUS_PORT) & 0x02);
    
    outb(KEYBOARD_DATA_PORT, led_byte);
}

void keyboard_update_leds(void) {
    keyboard_set_leds(caps_lock, num_lock, scroll_lock);
}

#ifdef SMOLOS_KEYBOARD_TEST

int main(void) {
    printf("Hello from keyboard.c!\n");
    
    return 0;
}

#endif