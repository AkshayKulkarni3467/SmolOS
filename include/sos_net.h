#ifndef INCLUDE_SMOLOS_NET_H
#define INCLUDE_SMOLOS_NET_H

#include "sos_stdint.h"

uint16_t checksum(void* vdata, int length);
void cmd_net_init(void);
void cmd_netinfo(void);
void send_arp_request(uint8_t* target_ip);
void cmd_ping(void);
void cmd_udp_send(void);
void cmd_netinfo(void);
void cmd_dns(void);
void cmd_tcp_ping(void);

#endif //INCLUDE_SMOLOS_NET_H