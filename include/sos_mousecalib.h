#ifndef INCLUDE_SMOLOS_MOUSE_CALIBRATION_H
#define INCLUDE_SMOLOS_MOUSE_CALIBRATION_H

#include "sos_stdint.h"

typedef enum {
    CALIBRATION_NOT_NEEDED,
    CALIBRATION_NEEDED,
    CALIBRATION_IN_PROGRESS,
    CALIBRATION_COMPLETE
} CalibrationState;

typedef struct {
    int center_x;
    int center_y;
    int is_calibrated;
    uint32_t calibration_timestamp;
} CalibrationData;

void mouse_calibration_init(void);
int mouse_needs_calibration(void);
void mouse_run_calibration(void);
CalibrationState mouse_get_calibration_state(void);
void mouse_reset_calibration(void);

void calibration_draw_screen(void);
void calibration_draw_target(int x, int y, int frame);
void calibration_draw_instructions(void);
void calibration_animate_target(void);
int calibration_wait_for_click(void);
void calibration_show_success(void);
void calibration_apply_offset(void);

void calibration_save_data(void);
void calibration_load_data(void);
CalibrationData* calibration_get_data(void);

#endif // INCLUDE_SMOLOS_MOUSE_CALIBRATION_H