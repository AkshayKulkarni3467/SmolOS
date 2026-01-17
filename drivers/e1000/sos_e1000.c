#include "sos_e1000.h"
#include "sos_io.h"
#include "sos_memory.h"
#include "sos_vga.h"

#define REG_CTRL    0x0000
#define REG_RCTL    0x0100
#define REG_TCTL    0x0400
#define REG_RDBAL   0x2800
#define REG_RDBAH   0x2804
#define REG_RDLEN   0x2808
#define REG_RDH     0x2810
#define REG_RDT     0x2818
#define REG_TDBAL   0x3800
#define REG_TDBAH   0x3804
#define REG_TDLEN   0x3808
#define REG_TDH     0x3810
#define REG_TDT     0x3818
#define REG_RAL     0x5400
#define REG_RAH     0x5404

#define NUM_RX_DESC 32
#define NUM_TX_DESC 8

typedef struct {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint16_t checksum;
    volatile uint8_t  status;
    volatile uint8_t  errors;
    volatile uint16_t special;
} __attribute__((packed)) rx_desc;

typedef struct {
    volatile uint64_t addr;
    volatile uint16_t length;
    volatile uint8_t  cso;
    volatile uint8_t  cmd;
    volatile uint8_t  status;
    volatile uint8_t  css;
    volatile uint16_t special;
} __attribute__((packed)) tx_desc;

static __attribute__((aligned(16))) rx_desc rx_descs[NUM_RX_DESC];
static __attribute__((aligned(16))) tx_desc tx_descs[NUM_TX_DESC];
static __attribute__((aligned(16))) uint8_t rx_buffers[NUM_RX_DESC][2048];
static __attribute__((aligned(16))) uint8_t tx_buffers[NUM_TX_DESC][2048];

static uint32_t mmio_base;
static uint8_t mac_addr[6];
static int rx_cur = 0;
static int tx_cur = 0;

static void e1000_write(uint16_t offset, uint32_t val) {
    *(volatile uint32_t*)(mmio_base + offset) = val;
}

static uint32_t e1000_read(uint16_t offset) {
    return *(volatile uint32_t*)(mmio_base + offset);
}

void e1000_init(PCIDevice* dev) {
    uint16_t cmd = pci_config_read_word(dev->bus, dev->device, dev->function, 0x04);
    pci_config_write(dev->bus, dev->device, dev->function, 0x04, cmd | 0x04);

    mmio_base = dev->bar[0] & 0xFFFFFFF0;

    uint32_t low = e1000_read(REG_RAL);
    uint32_t high = e1000_read(REG_RAH);
    mac_addr[0] = low & 0xFF;
    mac_addr[1] = (low >> 8) & 0xFF;
    mac_addr[2] = (low >> 16) & 0xFF;
    mac_addr[3] = (low >> 24) & 0xFF;
    mac_addr[4] = high & 0xFF;
    mac_addr[5] = (high >> 8) & 0xFF;

    for(int i = 0; i < NUM_RX_DESC; i++) {
        rx_descs[i].addr = (uint64_t)(uint32_t)rx_buffers[i];
        rx_descs[i].status = 0;
    }

    e1000_write(REG_RDBAL, (uint32_t)rx_descs);
    e1000_write(REG_RDBAH, 0);
    e1000_write(REG_RDLEN, NUM_RX_DESC * 16);
    e1000_write(REG_RDH, 0);
    e1000_write(REG_RDT, NUM_RX_DESC - 1);
    e1000_write(REG_RCTL, 0x0400801A); 

    for(int i = 0; i < NUM_TX_DESC; i++) {
        tx_descs[i].addr = (uint64_t)(uint32_t)tx_buffers[i];
        tx_descs[i].cmd = 0;
        tx_descs[i].status = 1; 
    }

    e1000_write(REG_TDBAL, (uint32_t)tx_descs);
    e1000_write(REG_TDBAH, 0);
    e1000_write(REG_TDLEN, NUM_TX_DESC * 16);
    e1000_write(REG_TDH, 0);
    e1000_write(REG_TDT, 0);
    e1000_write(REG_TCTL, 0x01000002); 
}

int e1000_send_packet(const void* data, uint16_t len) {
    tx_desc* desc = &tx_descs[tx_cur];

    while(!(desc->status & 0xFF)); 

    memcpy(tx_buffers[tx_cur], data, len);
    
    desc->length = len;
    desc->cmd = 0x09; 
    desc->status = 0;
    
    uint32_t old_cur = tx_cur;
    tx_cur = (tx_cur + 1) % NUM_TX_DESC;
    e1000_write(REG_TDT, tx_cur);
    
    return len;
}

int e1000_receive_packet(void* buffer, uint16_t max_len) {
    rx_desc* desc = &rx_descs[rx_cur];
    
    if((desc->status & 0x01)) { 
        uint16_t len = desc->length;
        if(len > max_len) len = max_len;
        
        memcpy(buffer, rx_buffers[rx_cur], len);
        
        desc->status = 0; 
        
        uint32_t old_cur = rx_cur;
        rx_cur = (rx_cur + 1) % NUM_RX_DESC;
        e1000_write(REG_RDT, old_cur);
        
        return len;
    }
    return 0;
}

uint8_t* e1000_get_mac(void) { return mac_addr; }