#ifndef INCLUDE_SMOLOS_FAT16_H
#define INCLUDE_SMOLOS_FAT16_H

#include "sos_stdint.h"

#define FILE_CONTENT_SIZE 4096  

typedef struct {
    char name[32];
    uint32_t size;
    uint8_t attributes;
    int is_directory;
} FAT16_FileInfo;


void fat16_init(void);


int fat16_create_file(const char* filename, const char* content, uint32_t size);
char* fat16_read_file(const char* filename, uint32_t* size_out);
int fat16_write_file(const char* filename, const char* content, uint32_t size);
int fat16_delete_file(const char* filename);


int fat16_list_files(FAT16_FileInfo* file_list, int max_files);


int fat16_file_exists(const char* filename);


uint32_t fat16_get_file_size(const char* filename);
uint32_t fat16_get_free_space(void);
uint32_t fat16_get_total_space(void);

#endif // INCLUDE_SMOLOS_FAT16_H