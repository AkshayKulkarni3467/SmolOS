#ifndef INCLUDE_SMOLOS_NET_STRUCTS_H
#define INCLUDE_SMOLOS_NET_STRUCTS_H

#include "sos_stdint.h"

#define ETHERNET_TYPE_ARP  0x0806
#define ETHERNET_TYPE_IP   0x0800
#define IP_PROTOCOL_ICMP   1
#define IP_PROTOCOL_UDP    17
#define IP_PROTOCOL_TCP 6

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

#define SWAP16(x) ((((x) & 0xFF) << 8) | (((x) >> 8) & 0xFF))
#define SWAP32(x) ((((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8) | (((x) >> 24) & 0xFF))

#define HTONS(x) SWAP16(x)
#define HTONL(x) SWAP32(x)
#define NTOHS(x) SWAP16(x)
#define NTOHL(x) SWAP32(x)

typedef struct {
    uint8_t dest[6];
    uint8_t src[6];
    uint16_t type;
} __attribute__((packed)) EthernetHeader;

typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t  hardware_size;
    uint8_t  protocol_size;
    uint16_t opcode;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) ARPHeader;

typedef struct {
    uint8_t  ihl : 4;
    uint8_t  version : 4;
    uint8_t  tos;
    uint16_t len;
    uint16_t id;
    uint16_t frag_offset;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) IPHeader;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t seq;
} __attribute__((packed)) ICMPHeader;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) UDPHeader;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t  data_offset; 
    uint8_t  flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) TCPHeader;

typedef struct {
    uint8_t li_vn_mode;      
    uint8_t stratum;         
    uint8_t poll;            
    uint8_t precision;       
    uint32_t root_delay;     
    uint32_t root_dispersion;
    uint32_t ref_id;        
    uint32_t ref_ts_sec;     
    uint32_t ref_ts_frac;   
    uint32_t orig_ts_sec;    
    uint32_t orig_ts_frac;
    uint32_t recv_ts_sec;    
    uint32_t recv_ts_frac;
    uint32_t trans_ts_sec;   
    uint32_t trans_ts_frac;
} __attribute__((packed)) NTPHeader;

#endif //INCLUDE_SMOLOS_NET_STRUCTS_H