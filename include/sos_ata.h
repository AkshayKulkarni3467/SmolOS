#ifndef INCLUDE_SMOLOS_ATA_H
#define INCLUDE_SMOLOS_ATA_H

#include "sos_stdint.h"

typedef struct {
    char model[41];          
    char serial[21];         
    uint32_t lba28_sectors; 
    int supports_lba;       
} ATA_IdentifyInfo;


void ata_init(void);


int ata_is_available(void);


int ata_read_sectors(uint32_t lba, uint8_t sector_count, void* buffer);
int ata_write_sectors(uint32_t lba, uint8_t sector_count, const void* buffer);
int ata_read_sector(uint32_t lba, void* buffer);
int ata_write_sector(uint32_t lba, const void* buffer);
int ata_identify(ATA_IdentifyInfo* info);

#endif // INCLUDE_SMOLOS_ATA_H