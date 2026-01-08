#include "sos_keyboard.h"
#include "sos_stdint.h"

#ifdef SMOLOS_KEYBOARD_TEST
#include "sos_stdio.h"
#endif

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64
#define BUFFER_SIZE 256

static volatile char key_buffer[BUFFER_SIZE];
static volatile uint8_t head = 0;
static volatile uint8_t tail = 0;
static volatile int has_key_press = 0;
static volatile int shift_pressed = 0;
static volatile int ctrl_pressed = 0;
static volatile int alt_pressed  = 0;
static volatile int caps_lock = 0;



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

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void keyboard_init(void) {
    //TODO Implement keyboard init func
}


#ifdef SMOLOS_KEYBOARD_TEST

int main(void) {
    printf("Hello from Keyboard!\n");
    return 0;
}

#endif