#include "sos_ata.h"
#include "sos_stdint.h"

#define ATA_PRIMARY_IO       0x1F0
#define ATA_PRIMARY_CONTROL  0x3F6
#define ATA_SECONDARY_IO     0x170
#define ATA_SECONDARY_CONTROL 0x376

#define ATA_REG_DATA         0x00
#define ATA_REG_ERROR        0x01
#define ATA_REG_FEATURES     0x01
#define ATA_REG_SECCOUNT     0x02
#define ATA_REG_LBA_LOW      0x03
#define ATA_REG_LBA_MID      0x04
#define ATA_REG_LBA_HIGH     0x05
#define ATA_REG_DRIVE_HEAD   0x06
#define ATA_REG_STATUS       0x07
#define ATA_REG_COMMAND      0x07

#define ATA_SR_BSY           0x80    
#define ATA_SR_DRDY          0x40    
#define ATA_SR_DF            0x20    
#define ATA_SR_DSC           0x10    
#define ATA_SR_DRQ           0x08    
#define ATA_SR_CORR          0x04    
#define ATA_SR_IDX           0x02    
#define ATA_SR_ERR           0x01    

#define ATA_CMD_READ_PIO     0x20
#define ATA_CMD_WRITE_PIO    0x30
#define ATA_CMD_IDENTIFY     0xEC
#define ATA_CMD_CACHE_FLUSH  0xE7

#define ATA_MASTER           0xE0  
#define ATA_SLAVE            0xF0

static uint16_t ata_base_port = ATA_PRIMARY_IO;
static uint16_t ata_control_port = ATA_PRIMARY_CONTROL;
static int ata_initialized = 0;

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void insw(uint16_t port, uint16_t* buffer, uint32_t word_count) {
    asm volatile(
        "cld\n\t"
        "rep insw"
        : "+D"(buffer), "+c"(word_count)
        : "d"(port)
        : "memory"
    );
}

static inline void outsw(uint16_t port, const uint16_t* buffer, uint32_t word_count) {
    asm volatile(
        "cld\n\t"
        "rep outsw"
        : "+S"(buffer), "+c"(word_count)
        : "d"(port)
        : "memory"
    );
}

static inline void ata_io_wait(void) {
    inb(ata_base_port + ATA_REG_STATUS);
    inb(ata_base_port + ATA_REG_STATUS);
    inb(ata_base_port + ATA_REG_STATUS);
    inb(ata_base_port + ATA_REG_STATUS);
}

static int ata_wait_ready(void) {
    uint8_t status;
    int timeout = 100000;
    
    while (timeout--) {
        status = inb(ata_base_port + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            return 0;  
        }
    }
    
    return -1; 
}

static int ata_wait_drq(void) {
    uint8_t status;
    int timeout = 100000;
    
    while (timeout--) {
        status = inb(ata_base_port + ATA_REG_STATUS);
        if (status & ATA_SR_DRQ) {
            return 0;  
        }
        if (status & ATA_SR_ERR) {
            return -1;  
        }
    }
    
    return -1; 
}

void ata_init(void) {
    if (ata_initialized) return;
    
    outb(ata_base_port + ATA_REG_DRIVE_HEAD, ATA_MASTER);
    ata_io_wait();
    
    for (volatile int i = 0; i < 10000; i++);
    
    if (ata_wait_ready() != 0) {
        ata_initialized = -1;  
        return;
    }
    
    ata_initialized = 1;
}

int ata_is_available(void) {
    return ata_initialized == 1;
}

int ata_read_sectors(uint32_t lba, uint8_t sector_count, void* buffer) {
    if (ata_initialized != 1) return -1;
    if (sector_count == 0) return 0;
    
    if (ata_wait_ready() != 0) {
        return -1;
    }
    
    outb(ata_base_port + ATA_REG_DRIVE_HEAD, 
         0xE0 | ((lba >> 24) & 0x0F));
    
    ata_io_wait();
    
    outb(ata_base_port + ATA_REG_SECCOUNT, sector_count);
    
    outb(ata_base_port + ATA_REG_LBA_LOW,  (uint8_t)(lba));
    outb(ata_base_port + ATA_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ata_base_port + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));
    
    outb(ata_base_port + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    uint16_t* buf = (uint16_t*)buffer;
    
    for (int i = 0; i < sector_count; i++) {
        if (ata_wait_drq() != 0) {
            return -1;
        }
        
        insw(ata_base_port + ATA_REG_DATA, buf, 256);
        buf += 256;  
        
        ata_io_wait();
    }
    
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t sector_count, const void* buffer) {
    if (ata_initialized != 1) return -1;
    if (sector_count == 0) return 0;
    
    if (ata_wait_ready() != 0) {
        return -1;
    }
    
    outb(ata_base_port + ATA_REG_DRIVE_HEAD, 
         0xE0 | ((lba >> 24) & 0x0F));
    
    ata_io_wait();
    
    outb(ata_base_port + ATA_REG_SECCOUNT, sector_count);
    
    outb(ata_base_port + ATA_REG_LBA_LOW,  (uint8_t)(lba));
    outb(ata_base_port + ATA_REG_LBA_MID,  (uint8_t)(lba >> 8));
    outb(ata_base_port + ATA_REG_LBA_HIGH, (uint8_t)(lba >> 16));
    
    outb(ata_base_port + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    const uint16_t* buf = (const uint16_t*)buffer;
    
    for (int i = 0; i < sector_count; i++) {
        if (ata_wait_drq() != 0) {
            return -1;
        }
        
        outsw(ata_base_port + ATA_REG_DATA, buf, 256);
        buf += 256;  
        
        ata_io_wait();
    }
    
    outb(ata_base_port + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    
    if (ata_wait_ready() != 0) {
        return -1;
    }
    
    return 0;
}

int ata_read_sector(uint32_t lba, void* buffer) {
    return ata_read_sectors(lba, 1, buffer);
}

int ata_write_sector(uint32_t lba, const void* buffer) {
    return ata_write_sectors(lba, 1, buffer);
}

int ata_identify(ATA_IdentifyInfo* info) {
    if (ata_initialized != 1) return -1;
    
    if (ata_wait_ready() != 0) {
        return -1;
    }
    
    outb(ata_base_port + ATA_REG_DRIVE_HEAD, ATA_MASTER);
    ata_io_wait();
    
    outb(ata_base_port + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    if (ata_wait_drq() != 0) {
        return -1;
    }
    
    uint16_t buffer[256];
    insw(ata_base_port + ATA_REG_DATA, buffer, 256);
    
    if (info) {
        for (int i = 0; i < 40; i += 2) {
            info->model[i] = (buffer[27 + i/2] >> 8) & 0xFF;
            info->model[i+1] = buffer[27 + i/2] & 0xFF;
        }
        info->model[40] = '\0';
        
        for (int i = 39; i >= 0; i--) {
            if (info->model[i] == ' ') {
                info->model[i] = '\0';
            } else {
                break;
            }
        }
        
        for (int i = 0; i < 20; i += 2) {
            info->serial[i] = (buffer[10 + i/2] >> 8) & 0xFF;
            info->serial[i+1] = buffer[10 + i/2] & 0xFF;
        }
        info->serial[20] = '\0';
        
        for (int i = 19; i >= 0; i--) {
            if (info->serial[i] == ' ') {
                info->serial[i] = '\0';
            } else {
                break;
            }
        }
        
        info->lba28_sectors = ((uint32_t)buffer[61] << 16) | buffer[60];
        
        info->supports_lba = (buffer[49] & 0x0200) != 0;
    }
    
    return 0;
}