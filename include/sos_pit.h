#ifndef INCLUDE_SMOLOS_PIT_H
#define INCLUDE_SMOLOS_PIT_H

#include "sos_stdint.h"
#include "sos_io.h"

#define PIT_FREQ 1193182
#define PIT_CMD  0x43
#define PIT_CH0  0x40
#define PIT_CH2  0x42

void delay(uint32_t ms);

#endif //INCLUDE_SMOLOS_PIT_H