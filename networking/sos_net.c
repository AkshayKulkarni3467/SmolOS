#include "sos_pci.h"
#include "sos_e1000.h"
#include "sos_net_structs.h"
#include "sos_vga.h"
#include "sos_memory.h"
#include "sos_string.h"
#include "sos_keyboard.h" 
#include "sos_io.h"
#include "sos_net.h"

// QEMU Network Defaults (User Mode)
// IP: 10.0.2.15 (0x0A00020F) -> Network Order: 0x0F02000A (LE machine view)
// However, using byte arrays is safer for endianness confusion.

static uint8_t my_ip[4] = {10, 0, 2, 15};
static uint8_t gateway_ip[4] = {10, 0, 2, 2};
static uint8_t dns_ip[4] = {10, 0, 2, 3};

static int net_initialized = 0;
static uint8_t gateway_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t target_mac_cache[6];
static int mac_resolved = 0;

int net_read_line(char* buffer, int max_length) {
    int pos = 0;
    while (1) {
        char c = wait_for_char(); 
        
        if (c == '\n') {
            vga_putchr('\n');
            buffer[pos] = '\0';
            return pos;
        } else if (c == '\b') {
            if (pos > 0) {
                vga_backspace();
                pos--;
            }
        } else if (c >= 32 && c < 127) {
            if (pos < max_length - 1) {
                vga_putchr(c);
                buffer[pos++] = c;
            }
        }
    }
}

uint16_t checksum(void* vdata, int length) {
    char* data = (char*)vdata;
    uint32_t acc = 0;
    for (int i = 0; i + 1 < length; i += 2) {
        uint16_t word;
        memcpy(&word, data + i, 2);
        acc += word;
    }
    if (length % 2 == 1) {
        uint16_t word = 0;
        memcpy(&word, data + length - 1, 1);
        acc += word;
    }
    while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);
    return (uint16_t)~acc;
}

uint32_t ip_to_int(uint8_t* ip) {
    return *(uint32_t*)ip;
}

void parse_ip(char* str, uint8_t* out) {
    int octet = 0;
    int val = 0;
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            val = val * 10 + (*str - '0');
        } else if (*str == '.') {
            out[octet++] = val;
            val = 0;
        }
        str++;
    }
    out[octet] = val;
}

int is_local_subnet(uint8_t* target) {
    return (target[0] == my_ip[0] && target[1] == my_ip[1] && target[2] == my_ip[2]);
}

int is_ip_string(char* str) {
    while (*str) {
        if ((*str < '0' || *str > '9') && *str != '.') return 0;
        str++;
    }
    return 1;
}


void send_arp_request(uint8_t* target_ip) {
    uint8_t packet[sizeof(EthernetHeader) + sizeof(ARPHeader)];
    EthernetHeader* eth = (EthernetHeader*)packet;
    ARPHeader* arp = (ARPHeader*)(packet + sizeof(EthernetHeader));
    uint8_t* my_mac = e1000_get_mac();

    memset(eth->dest, 0xFF, 6);
    memcpy(eth->src, my_mac, 6);
    eth->type = HTONS(ETHERNET_TYPE_ARP);

    arp->hardware_type = HTONS(1);
    arp->protocol_type = HTONS(ETHERNET_TYPE_IP);
    arp->hardware_size = 6;
    arp->protocol_size = 4;
    arp->opcode = HTONS(1);
    memcpy(arp->sender_mac, my_mac, 6);
    memcpy(&arp->sender_ip, my_ip, 4);
    memset(arp->target_mac, 0, 6);
    memcpy(&arp->target_ip, target_ip, 4);

    e1000_send_packet(packet, sizeof(packet));
}

int resolve_mac(uint8_t* target_ip) {
    mac_resolved = 0;

    uint8_t* arp_target = is_local_subnet(target_ip) ? target_ip : gateway_ip;
    
    vga_print("Resolving MAC for: ");
    vga_print_int(arp_target[0]); vga_print(".");
    vga_print_int(arp_target[1]); vga_print(".");
    vga_print_int(arp_target[2]); vga_print(".");
    vga_print_int(arp_target[3]); vga_println("...");

    send_arp_request(arp_target);

    for(volatile int i=0; i<5000000; i++) {
        uint8_t buf[1500];
        int len = e1000_receive_packet(buf, 1500);
        if (len > 0) {
            EthernetHeader* eth = (EthernetHeader*)buf;
            if (NTOHS(eth->type) == ETHERNET_TYPE_ARP) {
                ARPHeader* arp = (ARPHeader*)(buf + sizeof(EthernetHeader));
                if (NTOHS(arp->opcode) == 2) { 
                    if (memcmp(&arp->sender_ip, arp_target, 4) == 0) {
                        memcpy(target_mac_cache, arp->sender_mac, 6);
                        vga_println("ARP Resolved.");
                        mac_resolved = 1;
                        return 1;
                    }
                }
            }
        }
    }
    vga_println("ARP Failed.");
    return 0;
}


void cmd_net_init(void) {
    if (net_initialized) return;
    PCIDevice* dev = pci_find_device(0x8086, 0x100E);
    if (!dev) { vga_println("No e1000 card found."); return; }
    e1000_init(dev);
    net_initialized = 1;
    vga_println("Network (e1000) Initialized.");
}

void cmd_netinfo(void) {
    if (!net_initialized) { vga_println("Network not initialized."); return; }
    
    uint8_t* mac = e1000_get_mac();
    vga_print("MAC: ");
    for(int i=0; i<6; i++) {
        vga_print_hex(mac[i]);
        if(i<5) vga_print(":");
    }
    vga_println("");
    vga_println("IP: 10.0.2.15 (Static)");
    vga_println("Gateway: 10.0.2.2");
}


void resolve_mac_for_ip(uint32_t target_ip) {
    mac_resolved = 0;

    uint32_t arp_target_ip = is_local_subnet(target_ip) ? target_ip : gateway_ip;
    
    vga_print("Resolving MAC for ");
    vga_println(is_local_subnet(target_ip) ? "Local Device..." : "Gateway...");

    uint8_t packet[sizeof(EthernetHeader) + sizeof(ARPHeader)];
    EthernetHeader* eth = (EthernetHeader*)packet;
    ARPHeader* arp = (ARPHeader*)(packet + sizeof(EthernetHeader));
    uint8_t* my_mac = e1000_get_mac();

    memset(eth->dest, 0xFF, 6);
    memcpy(eth->src, my_mac, 6);
    eth->type = SWAP16(ETHERNET_TYPE_ARP);

    arp->hardware_type = SWAP16(1);
    arp->protocol_type = SWAP16(ETHERNET_TYPE_IP);
    arp->hardware_size = 6;
    arp->protocol_size = 4;
    arp->opcode = SWAP16(1);
    memcpy(arp->sender_mac, my_mac, 6);
    arp->sender_ip = my_ip;
    memset(arp->target_mac, 0, 6);
    arp->target_ip = arp_target_ip;

    e1000_send_packet(packet, sizeof(packet));

    for(volatile int i=0; i<10000000; i++) {
        uint8_t buf[1500];
        int len = e1000_receive_packet(buf, 1500);
        if (len > 0) {
            EthernetHeader* eth_in = (EthernetHeader*)buf;
            if (SWAP16(eth_in->type) == ETHERNET_TYPE_ARP) {
                ARPHeader* arp_in = (ARPHeader*)(buf + sizeof(EthernetHeader));
                if (SWAP16(arp_in->opcode) == 2 && arp_in->sender_ip == arp_target_ip) {
                    memcpy(target_mac_cache, arp_in->sender_mac, 6);
                    mac_resolved = 1;
                    return;
                }
            }
        }
    }
}


void cmd_ping(void) {
    if (!net_initialized) cmd_net_init();

    char ip_str[32];
    vga_print("Enter IP: ");
    if (net_read_line(ip_str, 32) == 0) return;

    uint8_t target_ip[4];
    parse_ip(ip_str, target_ip);

    if (!resolve_mac(target_ip)) return;

    vga_println("Pinging...");
    
    char* payload = "SmolOS Ping Payload Data 1234567"; 
    int payload_len = 32;
    int seq = 1;

    for (int i = 0; i < 4; i++) {
        int pkt_len = sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(ICMPHeader) + payload_len;
        uint8_t packet[pkt_len];

        EthernetHeader* eth = (EthernetHeader*)packet;
        IPHeader* ip = (IPHeader*)(packet + sizeof(EthernetHeader));
        ICMPHeader* icmp = (ICMPHeader*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader));
        char* data = (char*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(ICMPHeader));

        memcpy(eth->dest, target_mac_cache, 6);
        memcpy(eth->src, e1000_get_mac(), 6);
        eth->type = HTONS(ETHERNET_TYPE_IP);

        ip->version = 4;
        ip->ihl = 5;
        ip->tos = 0;
        ip->len = HTONS(sizeof(IPHeader) + sizeof(ICMPHeader) + payload_len);
        ip->id = HTONS(seq * 100);
        ip->frag_offset = 0;
        ip->ttl = 64;
        ip->proto = IP_PROTOCOL_ICMP;
        memcpy(&ip->src_ip, my_ip, 4);
        memcpy(&ip->dest_ip, target_ip, 4);
        ip->checksum = 0;
        ip->checksum = checksum(ip, sizeof(IPHeader));

        icmp->type = 8; 
        icmp->code = 0;
        icmp->id = HTONS(0x1337);
        icmp->seq = HTONS(seq);
        memcpy(data, payload, payload_len);
        icmp->checksum = 0;
        icmp->checksum = checksum(icmp, sizeof(ICMPHeader) + payload_len);

        e1000_send_packet(packet, pkt_len);

        int got_reply = 0;
        for(volatile int k=0; k<5000000; k++) {
            uint8_t buf[1500];
            int len = e1000_receive_packet(buf, 1500);
            if (len > 0) {
                EthernetHeader* eth_in = (EthernetHeader*)buf;
                if (NTOHS(eth_in->type) == ETHERNET_TYPE_IP) {
                    IPHeader* ip_in = (IPHeader*)(buf + sizeof(EthernetHeader));
                    if (ip_in->proto == IP_PROTOCOL_ICMP) {
                        ICMPHeader* icmp_in = (ICMPHeader*)(buf + sizeof(EthernetHeader) + sizeof(IPHeader));
                        if (icmp_in->type == 0 && NTOHS(icmp_in->seq) == seq) {
                            vga_print("Reply from "); vga_print(ip_str);
                            vga_print(" seq="); vga_print_int(seq);
                            vga_print(" ttl="); vga_print_int(ip_in->ttl);
                            vga_println("");
                            got_reply = 1;
                            break;
                        }
                    }
                }
            }
        }
        if(!got_reply) vga_println("Request timed out.");
        seq++;
        for(volatile int d=0; d<10000000; d++); 
    }
}

void cmd_udp_send(void) {
    if (!net_initialized) cmd_net_init();

    char ip_str[32];
    vga_print("Dest IP: ");
    if (net_read_line(ip_str, 32) == 0) return;
    uint8_t target_ip[4];
    parse_ip(ip_str, target_ip);

    char msg[64];
    vga_print("Message: ");
    net_read_line(msg, 64);
    int msg_len = 0; while(msg[msg_len]) msg_len++;

    if (!resolve_mac(target_ip)) return;

    int pkt_len = sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader) + msg_len;
    uint8_t packet[pkt_len];

    EthernetHeader* eth = (EthernetHeader*)packet;
    IPHeader* ip = (IPHeader*)(packet + sizeof(EthernetHeader));
    UDPHeader* udp = (UDPHeader*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader));
    char* data = (char*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader));

    memcpy(eth->dest, target_mac_cache, 6);
    memcpy(eth->src, e1000_get_mac(), 6);
    eth->type = HTONS(ETHERNET_TYPE_IP);

    ip->version = 4;
    ip->ihl = 5;
    ip->tos = 0;
    ip->len = HTONS(sizeof(IPHeader) + sizeof(UDPHeader) + msg_len);
    ip->id = HTONS(0x5555);
    ip->frag_offset = 0;
    ip->ttl = 64;
    ip->proto = IP_PROTOCOL_UDP;
    memcpy(&ip->src_ip, my_ip, 4);
    memcpy(&ip->dest_ip, target_ip, 4);
    ip->checksum = 0;
    ip->checksum = checksum(ip, sizeof(IPHeader));

    udp->src_port = HTONS(12345);
    udp->dest_port = HTONS(8080); 
    udp->length = HTONS(sizeof(UDPHeader) + msg_len);
    udp->checksum = 0; 

    memcpy(data, msg, msg_len);

    e1000_send_packet(packet, pkt_len);
    vga_println("UDP Packet Sent.");
}


int dns_name_to_wire(char* hostname, uint8_t* buffer) {
    int lock = 0; 
    int i = 0;    
    int pos = 1;  
    
    while (hostname[i] != '\0') {
        if (hostname[i] == '.') {
            buffer[lock] = pos - lock - 1; 
            lock = pos; 
            pos++; 
        } else {
            buffer[pos] = hostname[i];
            pos++;
        }
        i++;
    }
    buffer[lock] = pos - lock - 1; 
    buffer[pos++] = 0; 
    return pos;
}

void cmd_dns(void) {
    if (!net_initialized) cmd_net_init();
    
    char domain[64];
    vga_print("Domain: ");
    if (net_read_line(domain, 64) == 0) return;

    uint8_t resolved_ip[4];
    if (dns_resolve(domain, resolved_ip)) {
        vga_print("IP Address: ");
        vga_print_int(resolved_ip[0]); vga_print(".");
        vga_print_int(resolved_ip[1]); vga_print(".");
        vga_print_int(resolved_ip[2]); vga_print(".");
        vga_print_int(resolved_ip[3]); vga_println("");
    }
}

uint16_t tcp_checksum(IPHeader* ip, TCPHeader* tcp, int tcp_len) {
    uint32_t sum = 0;
    uint16_t* ip_src = (uint16_t*)&ip->src_ip;
    uint16_t* ip_dest = (uint16_t*)&ip->dest_ip;

    sum += ip_src[0]; sum += ip_src[1];
    sum += ip_dest[0]; sum += ip_dest[1];

    sum += HTONS(IP_PROTOCOL_TCP);

    sum += HTONS(tcp_len);

    uint16_t* tcp_ptr = (uint16_t*)tcp;
    for (int i = 0; i < tcp_len / 2; i++) {
        sum += tcp_ptr[i];
    }

    if (tcp_len % 2) {
        sum += (uint16_t)((uint8_t*)tcp)[tcp_len - 1] << 8;
    }

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    
    return ~sum;
}


int dns_resolve(char* domain, uint8_t* out_ip) {
    if (!resolve_mac(dns_ip)) return 0;

    vga_print("Resolving DNS for "); vga_print(domain); vga_println("...");

    uint8_t payload[128];
    payload[0] = 0xAA; payload[1] = 0xAA;
    payload[2] = 0x01; payload[3] = 0x00;
    payload[4] = 0x00; payload[5] = 0x01; 
    payload[6] = 0x00; payload[7] = 0x00; 
    payload[8] = 0x00; payload[9] = 0x00; 
    payload[10] = 0x00; payload[11] = 0x00;

    int name_len = dns_name_to_wire(domain, payload + 12);
    int cursor = 12 + name_len;
    payload[cursor++] = 0x00; payload[cursor++] = 0x01; 
    payload[cursor++] = 0x00; payload[cursor++] = 0x01; 

    int pkt_len = sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader) + cursor;
    uint8_t packet[pkt_len];

    EthernetHeader* eth = (EthernetHeader*)packet;
    IPHeader* ip = (IPHeader*)(packet + sizeof(EthernetHeader));
    UDPHeader* udp = (UDPHeader*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader));
    uint8_t* data = (uint8_t*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader));

    memcpy(eth->dest, target_mac_cache, 6);
    memcpy(eth->src, e1000_get_mac(), 6);
    eth->type = HTONS(ETHERNET_TYPE_IP);

    ip->version = 4; ip->ihl = 5; ip->tos = 0;
    ip->len = HTONS(sizeof(IPHeader) + sizeof(UDPHeader) + cursor);
    ip->id = HTONS(0xDDDD); ip->frag_offset = 0; ip->ttl = 64; ip->proto = IP_PROTOCOL_UDP;
    memcpy(&ip->src_ip, my_ip, 4);
    memcpy(&ip->dest_ip, dns_ip, 4);
    ip->checksum = 0; ip->checksum = checksum(ip, sizeof(IPHeader));

    udp->src_port = HTONS(54321);
    udp->dest_port = HTONS(53);
    udp->length = HTONS(sizeof(UDPHeader) + cursor);
    udp->checksum = 0;

    memcpy(data, payload, cursor);
    e1000_send_packet(packet, pkt_len);

    int timeout = 5000000;
    while (timeout--) {
        uint8_t buf[1500];
        int len = e1000_receive_packet(buf, 1500);
        if (len > 0) {
            EthernetHeader* in_eth = (EthernetHeader*)buf;
            if (HTONS(in_eth->type) == ETHERNET_TYPE_IP) {
                IPHeader* in_ip = (IPHeader*)(buf + sizeof(EthernetHeader));
                if (in_ip->proto == IP_PROTOCOL_UDP) {
                    UDPHeader* in_udp = (UDPHeader*)(buf + sizeof(EthernetHeader) + sizeof(IPHeader));
                    if (HTONS(in_udp->src_port) == 53) {
                        uint8_t* dns_resp = buf + sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(UDPHeader);
                        if (dns_resp[0] == 0xAA && dns_resp[1] == 0xAA) {
                            int answer_offset = 12 + name_len + 4;
                            uint8_t* ans = dns_resp + answer_offset;

                            if ((ans[0] & 0xC0) == 0xC0) ans += 2;
                            else { while(*ans != 0) ans++; ans++; }

                            uint16_t type = (ans[0] << 8) | ans[1];
                            uint16_t data_len = (ans[8] << 8) | ans[9];
                            
                            if (type == 1 && data_len == 4) {
                                memcpy(out_ip, ans + 10, 4);
                                return 1;
                            }
                        }
                    }
                }
            }
        }
    }
    vga_println("DNS Resolution Timed Out.");
    return 0;
}

void cmd_tcp_ping(void) {
    if (!net_initialized) cmd_net_init();

    char input[64];
    vga_print("Target (IP or Domain): ");
    if (net_read_line(input, 64) == 0) return;

    uint8_t target_ip[4];

    if (is_ip_string(input)) {
        parse_ip(input, target_ip);
    } else {

        if (!dns_resolve(input, target_ip)) {
            return; 
        }
    }

    if (!resolve_mac(target_ip)) return;

    vga_print("Sending TCP SYN to ");
    vga_print_int(target_ip[0]); vga_print(".");
    vga_print_int(target_ip[1]); vga_print(".");
    vga_print_int(target_ip[2]); vga_print(".");
    vga_print_int(target_ip[3]); vga_println(":80");

    int pkt_len = sizeof(EthernetHeader) + sizeof(IPHeader) + sizeof(TCPHeader);
    uint8_t packet[pkt_len];
    memset(packet, 0, pkt_len); 

    EthernetHeader* eth = (EthernetHeader*)packet;
    IPHeader* ip = (IPHeader*)(packet + sizeof(EthernetHeader));
    TCPHeader* tcp = (TCPHeader*)(packet + sizeof(EthernetHeader) + sizeof(IPHeader));

    memcpy(eth->dest, target_mac_cache, 6);
    memcpy(eth->src, e1000_get_mac(), 6);
    eth->type = HTONS(ETHERNET_TYPE_IP);

    ip->version = 4; ip->ihl = 5; ip->tos = 0;
    ip->len = HTONS(sizeof(IPHeader) + sizeof(TCPHeader));
    ip->id = HTONS(0xBEEF); ip->frag_offset = 0; ip->ttl = 64; 
    ip->proto = IP_PROTOCOL_TCP;
    memcpy(&ip->src_ip, my_ip, 4);
    memcpy(&ip->dest_ip, target_ip, 4);
    ip->checksum = 0; ip->checksum = checksum(ip, sizeof(IPHeader));

    uint16_t src_port = 40000 + (pit_get_total_milliseconds() % 10000);
    tcp->src_port = HTONS(src_port);
    tcp->dest_port = HTONS(80); 
    tcp->seq = HTONL(0x12345678);
    tcp->ack = 0;
    tcp->data_offset = 0x50; 
    tcp->flags = 0x02; 
    tcp->window = HTONS(8192);
    tcp->urgent_ptr = 0;
    tcp->checksum = 0;
    tcp->checksum = tcp_checksum(ip, tcp, sizeof(TCPHeader));

    e1000_send_packet(packet, pkt_len);

    int timeout = 5000000;
    int received = 0;
    
    while(timeout--) {
        uint8_t buf[1500];
        int len = e1000_receive_packet(buf, 1500);
        
        if (len > 0) {
            EthernetHeader* eth_in = (EthernetHeader*)buf;
            
            if (HTONS(eth_in->type) == ETHERNET_TYPE_IP) {
                IPHeader* ip_in = (IPHeader*)(buf + sizeof(EthernetHeader));

                if (ip_in->proto == IP_PROTOCOL_TCP && memcmp(&ip_in->src_ip, target_ip, 4) == 0) {
                    TCPHeader* tcp_in = (TCPHeader*)(buf + sizeof(EthernetHeader) + sizeof(IPHeader));
                    
                    if (NTOHS(tcp_in->dest_port) == src_port) {
                        vga_print("Response Received! Flags: [ ");
                        if (tcp_in->flags & 0x02) vga_print("SYN ");
                        if (tcp_in->flags & 0x10) vga_print("ACK ");
                        if (tcp_in->flags & 0x04) vga_print("RST ");
                        vga_println("]");
                        
                        if (tcp_in->flags & (0x02 | 0x10)) {
                            vga_println("Success: Port 80 is OPEN.");
                        } else if (tcp_in->flags & 0x04) {
                            vga_println("Success: Host is UP (but Port 80 closed).");
                        }
                        
                        received = 1;
                        break;
                    }
                }
            }
        }
    }
    
    if (!received) {
        vga_println("Timeout. Host may be down or firewall is dropping packets.");
    }
}
