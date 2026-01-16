#include "sos_mousecalib.h"
#include "sos_mouse.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_fat16.h"

static CalibrationState calibration_state = CALIBRATION_NEEDED;
static CalibrationData calibration_data;
static int animation_frame = 0;
static uint32_t last_animation_time = 0;

#define TARGET_X 40
#define TARGET_Y 12
#define ANIMATION_SPEED 200  
#define CALIBRATION_FILE "MOUSECAL.DAT"



void mouse_calibration_init(void) {
    calibration_data.center_x = TARGET_X;
    calibration_data.center_y = TARGET_Y;
    calibration_data.is_calibrated = 0;
    calibration_data.calibration_timestamp = 0;
    
    calibration_load_data();
    
    if (calibration_data.is_calibrated) {
        calibration_state = CALIBRATION_NOT_NEEDED;
    } else {
        calibration_state = CALIBRATION_NEEDED;
    }
    
    animation_frame = 0;
    last_animation_time = pit_get_total_milliseconds();
}

int mouse_needs_calibration(void) {
    return (calibration_state == CALIBRATION_NEEDED);
}

CalibrationState mouse_get_calibration_state(void) {
    return calibration_state;
}

void mouse_reset_calibration(void) {
    calibration_data.is_calibrated = 0;
    calibration_state = CALIBRATION_NEEDED;
    calibration_save_data();
}

CalibrationData* calibration_get_data(void) {
    return &calibration_data;
}


void mouse_run_calibration(void) {
    calibration_state = CALIBRATION_IN_PROGRESS;
    
    calibration_draw_screen();
    
    mouse_hide_cursor();
    
    int success = calibration_wait_for_click();
    
    if (success) {
        calibration_apply_offset();
        
        calibration_show_success();
        
        calibration_data.is_calibrated = 1;
        calibration_data.calibration_timestamp = pit_get_seconds();
        calibration_state = CALIBRATION_COMPLETE;
        
        calibration_save_data();
        
        mouse_show_cursor();
        
        pit_delay_ms(2000);
    } else {
        calibration_state = CALIBRATION_NEEDED;
    }
    vga_clear();
    vga_set_color(VGA_WHITE,VGA_BLCK);
}


void calibration_draw_screen(void) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(10, 2, 60, 20, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* title = "MOUSE CALIBRATION";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 4, title[i]);
    }
    
    calibration_draw_instructions();
    
    calibration_draw_target(TARGET_X, TARGET_Y, 0);
    
    vga_end_batch();
}

void calibration_draw_instructions(void) {
    const char* instructions[] = {
        "Welcome to Mouse Calibration!",
        "",
        "Please click on the CENTER of the",
        "pulsing target below to calibrate",
        "your mouse cursor position.",
        "",
        "This ensures accurate mouse tracking",
        "throughout SmolOS.",
        "",
        "Press ESC to skip calibration"
    };
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    for (int i = 0; i < 10; i++) {
        int line_len = 0;
        while (instructions[i][line_len]) line_len++;
        
        uint8_t color = (i == 0) ? VGA_LCYAN : 
                       (i == 9) ? VGA_DGREY : VGA_WHITE;
        vga_set_color(color, VGA_BLCK);
        
        for (int j = 0; instructions[i][j]; j++) {
            vga_putchr_at(40 - line_len / 2 + j, 6 + i, instructions[i][j]);
        }
    }
}

void calibration_draw_target(int x, int y, int frame) {
    vga_fill_rect(x - 5, y - 3, 11, 7, ' ', VGA_WHITE, VGA_BLCK);
    
    int size = frame % 3;
    
    uint8_t colors[] = {VGA_LRED, VGA_YELLOW, VGA_LGREEN};
    uint8_t color = colors[frame % 3];
    
    vga_set_color(color, VGA_BLCK);
    
    if (size == 0 || size == 2) {
        vga_putchr_at(x - 2, y - 1, '.');
        vga_putchr_at(x + 2, y - 1, '.');
        vga_putchr_at(x - 2, y + 1, '.');
        vga_putchr_at(x + 2, y + 1, '.');
        
        vga_putchr_at(x - 3, y, '.');
        vga_putchr_at(x + 3, y, '.');
        vga_putchr_at(x, y - 2, '.');
        vga_putchr_at(x, y + 2, '.');
    }
    
    if (size == 1 || size == 2) {
        vga_putchr_at(x - 1, y, '-');
        vga_putchr_at(x + 1, y, '-');
        vga_putchr_at(x, y - 1, '|');
        vga_putchr_at(x, y + 1, '|');
    }
    
    vga_set_color(VGA_LRED, VGA_BLCK);
    vga_putchr_at(x, y, 0xF9);  
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_putchr_at(x - 4, y - 2, 0xDA);  
    vga_putchr_at(x + 4, y - 2, 0xBF);  
    vga_putchr_at(x - 4, y + 2, 0xC0);  
    vga_putchr_at(x + 4, y + 2, 0xD9);  
    

    vga_set_color(VGA_DGREY, VGA_BLCK);
    char coord_text[30];
    int pos = 0;
    const char* label = "Target: (";
    for (int i = 0; label[i]; i++) coord_text[pos++] = label[i];
    

    if (x >= 10) {
        coord_text[pos++] = '0' + (x / 10);
        coord_text[pos++] = '0' + (x % 10);
    } else {
        coord_text[pos++] = '0' + x;
    }
    
    coord_text[pos++] = ',';
    coord_text[pos++] = ' ';
    
    if (y >= 10) {
        coord_text[pos++] = '0' + (y / 10);
        coord_text[pos++] = '0' + (y % 10);
    } else {
        coord_text[pos++] = '0' + y;
    }
    
    coord_text[pos++] = ')';
    coord_text[pos] = '\0';
    
    int len = 0;
    while (coord_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 19, coord_text[i]);
    }
}

void calibration_animate_target(void) {
    uint32_t current_time = pit_get_total_milliseconds();
    
    if (current_time - last_animation_time >= ANIMATION_SPEED) {
        animation_frame++;
        last_animation_time = current_time;
        
        calibration_draw_target(TARGET_X, TARGET_Y, animation_frame);
        vga_swap_buffers();
    }
}


int calibration_wait_for_click(void) {
    int clicked = 0;
    int cancelled = 0;
    
    MouseEvent event;
    
    while (!clicked && !cancelled) {
        calibration_animate_target();
        
        while (mouse_get_event(&event)) {
            if (event.type == MOUSE_EVENT_BUTTON_PRESS && 
                (event.buttons & MOUSE_LEFT_BUTTON)) {
                clicked = 1;
                
                calibration_data.center_x = event.x;
                calibration_data.center_y = event.y;
                break;
            }
        }
        
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            if (c == 27) {  
                cancelled = 1;
            }
        }
        
        pit_delay_ms(16);
    }
    
    return clicked;
}

void calibration_apply_offset(void) {
    int offset_x = calibration_data.center_x - TARGET_X;
    int offset_y = calibration_data.center_y - TARGET_Y;
    
    mouse_set_position(TARGET_X, TARGET_Y);
}

void calibration_show_success(void) {
    vga_begin_batch();
    
    vga_draw_box_double(20, 10, 40, 7, VGA_LGREEN, VGA_BLCK);
    vga_fill_rect(21, 11, 38, 5, ' ', VGA_YELLOW, VGA_GREEN);
    
    vga_set_color(VGA_YELLOW, VGA_GREEN);
    const char* success_text = " \x01 CALIBRATION COMPLETE! \x01 ";
    int len = 0;
    while (success_text[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 12, success_text[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_GREEN);
    const char* msg = "Mouse is now calibrated";
    len = 0;
    while (msg[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(40 - len / 2 + i, 14, msg[i]);
    }
    
    vga_end_batch();
}



void calibration_save_data(void) {
    typedef struct {
        int magic_number;      
        int center_x;
        int center_y;
        int is_calibrated;
        uint32_t calibration_timestamp;
        int checksum;
    } CalibrationFileData;
    
    CalibrationFileData file_data;
    file_data.magic_number = 0x4D43414C;
    file_data.center_x = calibration_data.center_x;
    file_data.center_y = calibration_data.center_y;
    file_data.is_calibrated = calibration_data.is_calibrated;
    file_data.calibration_timestamp = calibration_data.calibration_timestamp;
    
    file_data.checksum = file_data.magic_number + 
                         file_data.center_x + 
                         file_data.center_y + 
                         file_data.is_calibrated + 
                         file_data.calibration_timestamp;
    
    fat16_write_file(CALIBRATION_FILE, (const char*)&file_data, sizeof(CalibrationFileData));
}

void calibration_load_data(void) {
    if (!fat16_file_exists(CALIBRATION_FILE)) {
        calibration_data.center_x = TARGET_X;
        calibration_data.center_y = TARGET_Y;
        calibration_data.is_calibrated = 0;
        calibration_data.calibration_timestamp = 0;
        return;
    }
    
    uint32_t file_size;
    char* file_content = fat16_read_file(CALIBRATION_FILE, &file_size);
    
    if (!file_content || file_size == 0) {
        calibration_data.center_x = TARGET_X;
        calibration_data.center_y = TARGET_Y;
        calibration_data.is_calibrated = 0;
        calibration_data.calibration_timestamp = 0;
        return;
    }
    
    typedef struct {
        int magic_number;
        int center_x;
        int center_y;
        int is_calibrated;
        uint32_t calibration_timestamp;
        int checksum;
    } CalibrationFileData;
    
    if (file_size < sizeof(CalibrationFileData)) {
        calibration_data.center_x = TARGET_X;
        calibration_data.center_y = TARGET_Y;
        calibration_data.is_calibrated = 0;
        calibration_data.calibration_timestamp = 0;
        return;
    }
    
    CalibrationFileData* file_data = (CalibrationFileData*)file_content;
    
    if (file_data->magic_number != 0x4D43414C) {
        calibration_data.center_x = TARGET_X;
        calibration_data.center_y = TARGET_Y;
        calibration_data.is_calibrated = 0;
        calibration_data.calibration_timestamp = 0;
        return;
    }
    
    int calculated_checksum = file_data->magic_number + 
                             file_data->center_x + 
                             file_data->center_y + 
                             file_data->is_calibrated + 
                             file_data->calibration_timestamp;
    
    if (calculated_checksum != file_data->checksum) {
        calibration_data.center_x = TARGET_X;
        calibration_data.center_y = TARGET_Y;
        calibration_data.is_calibrated = 0;
        calibration_data.calibration_timestamp = 0;
        return;
    }
    
    if (file_data->center_x < 0 || file_data->center_x >= 80 ||
        file_data->center_y < 0 || file_data->center_y >= 25) {
        calibration_data.center_x = TARGET_X;
        calibration_data.center_y = TARGET_Y;
        calibration_data.is_calibrated = 0;
        calibration_data.calibration_timestamp = 0;
        return;
    }
    
    calibration_data.center_x = file_data->center_x;
    calibration_data.center_y = file_data->center_y;
    calibration_data.is_calibrated = file_data->is_calibrated;
    calibration_data.calibration_timestamp = file_data->calibration_timestamp;
}