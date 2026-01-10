#ifndef INCLUDE_SMOLOS_FILE_MANAGER_H
#define INCLUDE_SMOLOS_FILE_MANAGER_H

#include "stdint.h"

void extract_filename(const char* command, char* filename);
void format_size(uint32_t size, char* output);
void text_editor(const char* filename);
void file_viewer(const char* filename);
void file_manager_command(void);

#endif //INCLUDE_SMOLOS_FILE_MANAGER_H
