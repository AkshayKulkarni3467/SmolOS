#ifndef INCLUDE_SMOLOS_E1000_H
#define INCLUDE_SMOLOS_E1000_H

#include "sos_stdint.h"
#include "sos_pci.h"

void e1000_init(PCIDevice* dev);
int e1000_send_packet(const void* data, uint16_t len);
int e1000_receive_packet(void* buffer, uint16_t max_len);
uint8_t* e1000_get_mac(void);

#endif //INCLUDE_SMOLOS_E1000_H