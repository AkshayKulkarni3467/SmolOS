#ifndef INCLUDE_SMOLOS_DIV64_H
#define INCLUDE_SMOLOS_DIV64_H

#include "sos_stdint.h"

uint64_t __udivdi3(uint64_t num, uint64_t den);
uint64_t __umoddi3(uint64_t num, uint64_t den);

#endif //INCLUDE_SMOLOS_DIV64_H