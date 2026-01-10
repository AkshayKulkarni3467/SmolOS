#include "sos_fat16.h"
#include "sos_string.h"
#include "sos_memory.h"
#include "sos_vga.h"
#include "sos_ata.h"

#define DISK_SIZE (1024 * 1024 * 2)  

static int use_real_disk = 1;  
static uint8_t virtual_disk[DISK_SIZE];  

typedef struct __attribute__((packed)) {
    uint8_t  jump_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} FAT16_BootSector;

typedef struct __attribute__((packed)) {
    uint8_t  name[11];          
    uint8_t  attributes;         
    uint8_t  reserved;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high; 
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} FAT16_DirEntry;

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

static FAT16_BootSector* boot_sector;
static uint16_t* fat_table;
static FAT16_DirEntry* root_directory;
static int fat16_initialized = 0;

#define BYTES_PER_SECTOR      512
#define SECTORS_PER_CLUSTER   4
#define RESERVED_SECTORS      1
#define NUM_FATS              2
#define ROOT_ENTRY_COUNT      224
#define TOTAL_SECTORS         4096  

static uint32_t fat_size;
static uint32_t root_dir_sectors;
static uint32_t first_data_sector;
static uint32_t data_sectors;
static uint32_t total_clusters;


static void fat16_read_sector(uint32_t sector, void* buffer) {
    if (use_real_disk && ata_is_available()) {
        if (ata_read_sector(sector, buffer) != 0) {
            memcpy(buffer, virtual_disk + (sector * BYTES_PER_SECTOR), BYTES_PER_SECTOR);
        }
    } else {
        memcpy(buffer, virtual_disk + (sector * BYTES_PER_SECTOR), BYTES_PER_SECTOR);
    }
}

static void fat16_write_sector(uint32_t sector, const void* buffer) {
    if (use_real_disk && ata_is_available()) {
        if (ata_write_sector(sector, buffer) != 0) {
            memcpy(virtual_disk + (sector * BYTES_PER_SECTOR), buffer, BYTES_PER_SECTOR);
        }
    } else {
        memcpy(virtual_disk + (sector * BYTES_PER_SECTOR), buffer, BYTES_PER_SECTOR);
    }
}

static uint16_t fat16_get_fat_entry(uint16_t cluster) {
    if (cluster >= total_clusters) return 0xFFFF;
    return fat_table[cluster];
}

static void fat16_set_fat_entry(uint16_t cluster, uint16_t value) {
    if (cluster >= total_clusters) return;
    fat_table[cluster] = value;
    
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = RESERVED_SECTORS + (fat_offset / BYTES_PER_SECTOR);
    uint32_t ent_offset = fat_offset % BYTES_PER_SECTOR;
    
    uint8_t sector_buffer[BYTES_PER_SECTOR];
    
    for (int i = 0; i < NUM_FATS; i++) {
        fat16_read_sector(fat_sector + (i * fat_size), sector_buffer);
        *(uint16_t*)(sector_buffer + ent_offset) = value;
        fat16_write_sector(fat_sector + (i * fat_size), sector_buffer);
    }
}

static uint16_t fat16_find_free_cluster(void) {
    for (uint16_t i = 2; i < total_clusters; i++) {
        if (fat16_get_fat_entry(i) == 0) {
            return i;
        }
    }
    return 0xFFFF;  
}

static uint32_t fat16_cluster_to_sector(uint16_t cluster) {
    return first_data_sector + ((cluster - 2) * SECTORS_PER_CLUSTER);
}

static void fat16_format_filename(const char* input, char* output) {
    memset(output, ' ', 11);
    
    int i = 0;
    int out_pos = 0;
    
    while (input[i] && input[i] != '.' && out_pos < 8) {
        output[out_pos++] = (input[i] >= 'a' && input[i] <= 'z') ? 
                            input[i] - 32 : input[i];
        i++;
    }
    
    while (input[i] && input[i] != '.') i++;
    
    if (input[i] == '.') {
        i++;
        out_pos = 8;
        while (input[i] && out_pos < 11) {
            output[out_pos++] = (input[i] >= 'a' && input[i] <= 'z') ? 
                                input[i] - 32 : input[i];
            i++;
        }
    }
}

static int fat16_compare_filename(const char* fat_name, const char* search_name) {
    char formatted[11];
    fat16_format_filename(search_name, formatted);
    return memcmp(fat_name, formatted, 11) == 0;
}

static void fat16_unformat_filename(const char* fat_name, char* output) {
    int i, j = 0;
    
    for (i = 0; i < 8 && fat_name[i] != ' '; i++) {
        output[j++] = fat_name[i];
    }
    
    int has_ext = 0;
    for (i = 8; i < 11; i++) {
        if (fat_name[i] != ' ') {
            has_ext = 1;
            break;
        }
    }
    
    if (has_ext) {
        output[j++] = '.';
        for (i = 8; i < 11 && fat_name[i] != ' '; i++) {
            output[j++] = fat_name[i];
        }
    }
    
    output[j] = '\0';
}

void fat16_init(void) {
    if (fat16_initialized) return;
    
    ata_init();
    
    if (ata_is_available()) {
        use_real_disk = 1;
        
        uint8_t boot_buffer[BYTES_PER_SECTOR];
        if (ata_read_sector(0, boot_buffer) == 0) {
            FAT16_BootSector* existing_boot = (FAT16_BootSector*)boot_buffer;
            
            if (boot_buffer[510] == 0x55 && boot_buffer[511] == 0xAA &&
                existing_boot->bytes_per_sector == BYTES_PER_SECTOR) {
                
                boot_sector = (FAT16_BootSector*)virtual_disk;
                memcpy(boot_sector, existing_boot, BYTES_PER_SECTOR);
                
                fat_size = boot_sector->fat_size_16;
                root_dir_sectors = ((boot_sector->root_entry_count * 32) + (BYTES_PER_SECTOR - 1)) / BYTES_PER_SECTOR;
                first_data_sector = RESERVED_SECTORS + (NUM_FATS * fat_size) + root_dir_sectors;
                data_sectors = boot_sector->total_sectors_16 - first_data_sector;
                total_clusters = data_sectors / boot_sector->sectors_per_cluster;
                
                fat_table = (uint16_t*)(virtual_disk + (RESERVED_SECTORS * BYTES_PER_SECTOR));
                for (uint32_t i = 0; i < fat_size; i++) {
                    ata_read_sector(RESERVED_SECTORS + i, 
                                   (uint8_t*)fat_table + (i * BYTES_PER_SECTOR));
                }
                
                root_directory = (FAT16_DirEntry*)(virtual_disk + 
                    ((RESERVED_SECTORS + (NUM_FATS * fat_size)) * BYTES_PER_SECTOR));
                for (uint32_t i = 0; i < root_dir_sectors; i++) {
                    ata_read_sector(RESERVED_SECTORS + (NUM_FATS * fat_size) + i,
                                   (uint8_t*)root_directory + (i * BYTES_PER_SECTOR));
                }
                
                fat16_initialized = 1;
                return;
            }
        }
        
    } else {
        // Use RAM :)
        use_real_disk = 0;
    }
    
    memset(virtual_disk, 0, DISK_SIZE);
    
    boot_sector = (FAT16_BootSector*)virtual_disk;
    
    boot_sector->jump_boot[0] = 0xEB;
    boot_sector->jump_boot[1] = 0x3C;
    boot_sector->jump_boot[2] = 0x90;
    
    memcpy(boot_sector->oem_name, "SMOLOS  ", 8);
    boot_sector->bytes_per_sector = BYTES_PER_SECTOR;
    boot_sector->sectors_per_cluster = SECTORS_PER_CLUSTER;
    boot_sector->reserved_sectors = RESERVED_SECTORS;
    boot_sector->num_fats = NUM_FATS;
    boot_sector->root_entry_count = ROOT_ENTRY_COUNT;
    boot_sector->total_sectors_16 = TOTAL_SECTORS;
    boot_sector->media_type = 0xF8;
    
    fat_size = ((TOTAL_SECTORS * 2) + (BYTES_PER_SECTOR - 1)) / BYTES_PER_SECTOR;
    boot_sector->fat_size_16 = fat_size;
    
    boot_sector->sectors_per_track = 63;
    boot_sector->num_heads = 16;
    boot_sector->hidden_sectors = 0;
    boot_sector->total_sectors_32 = 0;
    boot_sector->drive_number = 0x80;
    boot_sector->boot_signature = 0x29;
    boot_sector->volume_id = 0x12345678;
    
    memcpy(boot_sector->volume_label, "SMOLOS     ", 11);
    memcpy(boot_sector->fs_type, "FAT16   ", 8);
    
    virtual_disk[510] = 0x55;
    virtual_disk[511] = 0xAA;
    
    root_dir_sectors = ((ROOT_ENTRY_COUNT * 32) + (BYTES_PER_SECTOR - 1)) / BYTES_PER_SECTOR;
    first_data_sector = RESERVED_SECTORS + (NUM_FATS * fat_size) + root_dir_sectors;
    data_sectors = TOTAL_SECTORS - first_data_sector;
    total_clusters = data_sectors / SECTORS_PER_CLUSTER;
    
    fat_table = (uint16_t*)(virtual_disk + (RESERVED_SECTORS * BYTES_PER_SECTOR));
    
    fat_table[0] = 0xFF00 | boot_sector->media_type;
    fat_table[1] = 0xFFFF;
    
    root_directory = (FAT16_DirEntry*)(virtual_disk + 
        ((RESERVED_SECTORS + (NUM_FATS * fat_size)) * BYTES_PER_SECTOR));
    
    if (use_real_disk) {
        ata_write_sector(0, virtual_disk);
        
        for (uint32_t i = 0; i < fat_size; i++) {
            ata_write_sector(RESERVED_SECTORS + i, 
                           (uint8_t*)fat_table + (i * BYTES_PER_SECTOR));
            ata_write_sector(RESERVED_SECTORS + fat_size + i, 
                           (uint8_t*)fat_table + (i * BYTES_PER_SECTOR));
        }
        
        for (uint32_t i = 0; i < root_dir_sectors; i++) {
            ata_write_sector(RESERVED_SECTORS + (NUM_FATS * fat_size) + i,
                           (uint8_t*)root_directory + (i * BYTES_PER_SECTOR));
        }
    }
    
    fat16_create_file("README.TXT", "Welcome to SmolOS FAT16 File System!\n", 38);
    fat16_create_file("NOTES.TXT", "Your personal notes file.\n", 26);
    
    fat16_initialized = 1;
}

int fat16_create_file(const char* filename, const char* content, uint32_t size) {
    if (!fat16_initialized) return -1;
    
    for (int i = 0; i < ROOT_ENTRY_COUNT; i++) {
        if (root_directory[i].name[0] == 0) break;
        if (root_directory[i].name[0] == 0xE5) continue;
        
        if (fat16_compare_filename((char*)root_directory[i].name, filename)) {
            return -2;  
        }
        
    }
    
    int dir_entry = -1;
    for (int i = 0; i < ROOT_ENTRY_COUNT; i++) {
        if (root_directory[i].name[0] == 0 || root_directory[i].name[0] == 0xE5) {
            dir_entry = i;
            break;
        }
    }
    
    if (dir_entry == -1) return -3;  
    
    uint16_t first_cluster = 0;
    if (size > 0) {
        first_cluster = fat16_find_free_cluster();
        if (first_cluster == 0xFFFF) return -4;  
        
        uint32_t bytes_written = 0;
        uint16_t current_cluster = first_cluster;
        
        while (bytes_written < size) {
            uint32_t sector = fat16_cluster_to_sector(current_cluster);
            uint32_t bytes_to_write = (size - bytes_written > BYTES_PER_SECTOR * SECTORS_PER_CLUSTER) ?
                                      BYTES_PER_SECTOR * SECTORS_PER_CLUSTER :
                                      size - bytes_written;
            
            for (int s = 0; s < SECTORS_PER_CLUSTER && bytes_written < size; s++) {
                uint8_t sector_buffer[BYTES_PER_SECTOR];
                memset(sector_buffer, 0, BYTES_PER_SECTOR);
                
                uint32_t copy_size = (size - bytes_written > BYTES_PER_SECTOR) ?
                                     BYTES_PER_SECTOR : size - bytes_written;
                
                memcpy(sector_buffer, content + bytes_written, copy_size);
                fat16_write_sector(sector + s, sector_buffer);
                bytes_written += copy_size;
            }
            
            if (bytes_written < size) {
                uint16_t next_cluster = fat16_find_free_cluster();
                if (next_cluster == 0xFFFF) break;  
                
                fat16_set_fat_entry(current_cluster, next_cluster);
                current_cluster = next_cluster;
            } else {
                fat16_set_fat_entry(current_cluster, 0xFFFF);  
            }
        }
    }
    
    FAT16_DirEntry* entry = &root_directory[dir_entry];
    fat16_format_filename(filename, (char*)entry->name);
    entry->attributes = ATTR_ARCHIVE;
    entry->reserved = 0;
    entry->creation_time_tenth = 0;
    entry->creation_time = 0;
    entry->creation_date = 0x2821;  
    entry->last_access_date = 0x2821;
    entry->first_cluster_high = 0;
    entry->last_write_time = 0;
    entry->last_write_date = 0x2821;
    entry->first_cluster_low = first_cluster;
    entry->file_size = size;
    
    fat16_sync_root_dir();
    if (first_cluster > 0) {
        fat16_sync_fat();
    }

    return 0;
}

char* fat16_read_file(const char* filename, uint32_t* size_out) {
    if (!fat16_initialized) return 0;
    
    FAT16_DirEntry* entry = 0;
    for (int i = 0; i < ROOT_ENTRY_COUNT; i++) {
        if (root_directory[i].name[0] == 0) break;
        if (root_directory[i].name[0] == 0xE5) continue;
        
        if (fat16_compare_filename((char*)root_directory[i].name, filename)) {
            entry = &root_directory[i];
            break;
        }
    }
    
    if (!entry) return 0;
    
    static char file_buffer[FILE_CONTENT_SIZE];
    memset(file_buffer, 0, FILE_CONTENT_SIZE);
    
    if (size_out) *size_out = entry->file_size;
    
    if (entry->file_size == 0) return file_buffer;
    
    uint32_t bytes_read = 0;
    uint16_t current_cluster = entry->first_cluster_low;
    
    //Umm find another way to prevent loops?
    int max_iterations = 1000;
    
    while (current_cluster >= 2 && current_cluster < 0xFFF8 && bytes_read < entry->file_size && max_iterations-- > 0) {
        uint32_t sector = fat16_cluster_to_sector(current_cluster);
        
        for (int s = 0; s < SECTORS_PER_CLUSTER && bytes_read < entry->file_size; s++) {
            uint8_t sector_buffer[BYTES_PER_SECTOR];
            fat16_read_sector(sector + s, sector_buffer);
            
            uint32_t copy_size = (entry->file_size - bytes_read > BYTES_PER_SECTOR) ?
                                BYTES_PER_SECTOR : entry->file_size - bytes_read;
            
            memcpy(file_buffer + bytes_read, sector_buffer, copy_size);
            bytes_read += copy_size;
        }
        
        current_cluster = fat16_get_fat_entry(current_cluster);
    }
    
    file_buffer[bytes_read] = '\0';
    return file_buffer;
}

int fat16_write_file(const char* filename, const char* content, uint32_t size) {
    if (!fat16_initialized) return -1;
    
    if (fat16_file_exists(filename)) {
        fat16_delete_file(filename);
    }
    
    return fat16_create_file(filename, content, size);
}

int fat16_delete_file(const char* filename) {
    if (!fat16_initialized) return -1;
    
    FAT16_DirEntry* entry = 0;
    int entry_index = -1;
    
    for (int i = 0; i < ROOT_ENTRY_COUNT; i++) {
        if (root_directory[i].name[0] == 0) break;
        if (root_directory[i].name[0] == 0xE5) continue;
        
        if (fat16_compare_filename((char*)root_directory[i].name, filename)) {
            entry = &root_directory[i];
            entry_index = i;
            break;
        }
    }
    
    if (!entry) return -2;
    
    uint16_t current_cluster = entry->first_cluster_low;
    
    if (current_cluster >= 2 && current_cluster < 0xFFF8) {
        int max_iterations = 1000;
        
        while (current_cluster >= 2 && current_cluster < 0xFFF8 && max_iterations-- > 0) {
            uint16_t next_cluster = fat16_get_fat_entry(current_cluster);
            fat16_set_fat_entry(current_cluster, 0);
            current_cluster = next_cluster;
        }
    }
    
    entry->name[0] = 0xE5;

    fat16_sync_fat();
    fat16_sync_root_dir();
    
    return 0;
}

int fat16_list_files(FAT16_FileInfo* file_list, int max_files) {
    if (!fat16_initialized) return 0;
    
    int count = 0;
    for (int i = 0; i < ROOT_ENTRY_COUNT && count < max_files; i++) {
        if (root_directory[i].name[0] == 0) break;
        if (root_directory[i].name[0] == 0xE5) continue;
        if (root_directory[i].attributes & ATTR_VOLUME_ID) continue;
        
        fat16_unformat_filename((char*)root_directory[i].name, file_list[count].name);
        file_list[count].size = root_directory[i].file_size;
        file_list[count].attributes = root_directory[i].attributes;
        file_list[count].is_directory = (root_directory[i].attributes & ATTR_DIRECTORY) != 0;
        count++;
    }
    
    return count;
}

int fat16_file_exists(const char* filename) {
    if (!fat16_initialized) return 0;
    
    for (int i = 0; i < ROOT_ENTRY_COUNT; i++) {
        if (root_directory[i].name[0] == 0) break;
        if (root_directory[i].name[0] == 0xE5) continue;
        
        if (fat16_compare_filename((char*)root_directory[i].name, filename)) {
            return 1;
        }
    }
    
    return 0;
}

uint32_t fat16_get_file_size(const char* filename) {
    if (!fat16_initialized) return 0;
    
    for (int i = 0; i < ROOT_ENTRY_COUNT; i++) {
        if (root_directory[i].name[0] == 0) break;
        if (root_directory[i].name[0] == 0xE5) continue;
        
        if (fat16_compare_filename((char*)root_directory[i].name, filename)) {
            return root_directory[i].file_size;
        }
    }
    
    return 0;
}

uint32_t fat16_get_free_space(void) {
    if (!fat16_initialized) return 0;
    
    uint32_t free_clusters = 0;
    for (uint16_t i = 2; i < total_clusters; i++) {
        if (fat16_get_fat_entry(i) == 0) {
            free_clusters++;
        }
    }
    
    return free_clusters * SECTORS_PER_CLUSTER * BYTES_PER_SECTOR;
}

uint32_t fat16_get_total_space(void) {
    return total_clusters * SECTORS_PER_CLUSTER * BYTES_PER_SECTOR;
}

void fat16_sync_fat(void) {
    if (!use_real_disk || !ata_is_available()) return;
    
    for (uint32_t i = 0; i < fat_size; i++) {
        ata_write_sector(RESERVED_SECTORS + i, 
                       (uint8_t*)fat_table + (i * BYTES_PER_SECTOR));
        ata_write_sector(RESERVED_SECTORS + fat_size + i, 
                       (uint8_t*)fat_table + (i * BYTES_PER_SECTOR));
    }
}

void fat16_sync_root_dir(void) {
    if (!use_real_disk || !ata_is_available()) return;
    
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_write_sector(RESERVED_SECTORS + (NUM_FATS * fat_size) + i,
                       (uint8_t*)root_directory + (i * BYTES_PER_SECTOR));
    }
}

int fat16_using_real_disk(void) {
    return use_real_disk && ata_is_available();
}