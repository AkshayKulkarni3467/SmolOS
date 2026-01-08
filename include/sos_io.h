#ifndef INCLUDE_SMOLOS_IO_H
#define INCLUDE_SMOLOS_IO_H

#include "sos_stdint.h"

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#endif //INCLUDE_SMOLOS_IO_H