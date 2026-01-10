#include "sos_filemanager.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_fat16.h"
#include "sos_string.h"
#include "sos_memory.h"

#define EDITOR_BUFFER_SIZE 4096
#define MAX_LINES 100
#define LINE_LENGTH 80

typedef struct {
    char buffer[EDITOR_BUFFER_SIZE];
    int cursor_row;
    int cursor_col;
    int scroll_offset;
    int buffer_size;
    int modified;
    char filename[32];
} EditorState;

static EditorState editor;

void extract_filename(const char* command, char* filename) {
    const char* ptr = command;
    while (*ptr && *ptr != ' ') ptr++;
    while (*ptr && *ptr == ' ') ptr++;
    
    int i = 0;
    while (*ptr && *ptr != ' ' && i < 31) {
        filename[i++] = *ptr++;
    }
    filename[i] = '\0';
}

void format_size(uint32_t size, char* output) {
    if (size < 1024) {
        int i = 0;
        if (size == 0) {
            output[i++] = '0';
        } else {
            char temp[16];
            int j = 0;
            while (size > 0) {
                temp[j++] = '0' + (size % 10);
                size /= 10;
            }
            while (j > 0) {
                output[i++] = temp[--j];
            }
        }
        output[i++] = ' ';
        output[i++] = 'B';
        output[i] = '\0';
    } else if (size < 1024 * 1024) {
        uint32_t kb = size / 1024;
        int i = 0;
        char temp[16];
        int j = 0;
        while (kb > 0) {
            temp[j++] = '0' + (kb % 10);
            kb /= 10;
        }
        while (j > 0) {
            output[i++] = temp[--j];
        }
        output[i++] = ' ';
        output[i++] = 'K';
        output[i++] = 'B';
        output[i] = '\0';
    } else {
        uint32_t mb = size / (1024 * 1024);
        int i = 0;
        char temp[16];
        int j = 0;
        while (mb > 0) {
            temp[j++] = '0' + (mb % 10);
            mb /= 10;
        }
        while (j > 0) {
            output[i++] = temp[--j];
        }
        output[i++] = ' ';
        output[i++] = 'M';
        output[i++] = 'B';
        output[i] = '\0';
    }
}

//TODO Debug text editor

void text_editor(const char* filename) {
    memset(&editor, 0, sizeof(EditorState));
    strcpy(editor.filename, (char*)filename);
    
    uint32_t file_size;
    char* content = fat16_read_file(filename, &file_size);
    
    if (content && file_size > 0) {
        memcpy(editor.buffer, content, file_size < EDITOR_BUFFER_SIZE ? file_size : EDITOR_BUFFER_SIZE - 1);
        editor.buffer_size = file_size;
        editor.buffer[editor.buffer_size] = '\0';
    }
    
    int running = 1;
    int need_redraw = 1;
    
    while (running) {
        if (need_redraw) {
            vga_begin_batch();
            vga_clear();
            
            vga_fill_rect(0, 0, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
            vga_set_color(VGA_WHITE, VGA_BLUE);
            vga_print_centered("SmolOS Text Editor", 0);
            
            vga_fill_rect(0, 1, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
            vga_set_color(VGA_WHITE, VGA_DGREY);
            vga_putchr_at(1, 1, 0);
            vga_print("File: ");
            vga_print(editor.filename);
            
            if (editor.modified) {
                vga_print(" [Modified]");
            }
            
            vga_set_color(VGA_WHITE, VGA_BLCK);
            int display_row = 3;
            int char_idx = 0;
            int current_line = 0;
            int current_col = 0;
            
            while (char_idx < editor.buffer_size && current_line < editor.scroll_offset) {
                if (editor.buffer[char_idx] == '\n') {
                    current_line++;
                }
                char_idx++;
            }
            
            current_col = 0;
            while (char_idx < editor.buffer_size && display_row < 23) {
                char c = editor.buffer[char_idx];
                
                if (c == '\n') {
                    display_row++;
                    current_col = 0;
                    current_line++;
                } else if (c >= 32 && c < 127) {
                    vga_putchr_at(current_col, display_row, c);
                    current_col++;
                    if (current_col >= 80) {
                        current_col = 0;
                        display_row++;
                    }
                }
                
                char_idx++;
            }
            
            int cursor_display_row = 3 + (editor.cursor_row - editor.scroll_offset);
            if (cursor_display_row >= 3 && cursor_display_row < 23) {
                vga_putchr_at(editor.cursor_col, cursor_display_row, '_');
            }
            
            vga_fill_rect(0, 23, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
            vga_set_color(VGA_YELLOW, VGA_DGREY);
            vga_putchr_at(1, 23, 0);
            vga_print("F1:Save F2:Save&Exit F3:Exit ESC:Abandon");
            
            vga_fill_rect(0, 24, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
            vga_set_color(VGA_WHITE, VGA_DGREY);
            vga_putchr_at(1, 24, 0);
            vga_print("Size: ");
            char size_str[32];
            format_size(editor.buffer_size, size_str);
            vga_print(size_str);
            
            vga_putchr_at(20, 24, 0);
            vga_print("Line: ");
            vga_print_int(editor.cursor_row + 1);
            
            vga_putchr_at(35, 24, 0);
            vga_print("Col: ");
            vga_print_int(editor.cursor_col + 1);
            
            vga_end_batch();
            need_redraw = 0;
        }
        
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            
            if (c == CHAR_F1) {
                fat16_write_file(editor.filename, editor.buffer, editor.buffer_size);
                editor.modified = 0;
                need_redraw = 1;
            }
            else if (c == CHAR_F2) {
                fat16_write_file(editor.filename, editor.buffer, editor.buffer_size);
                running = 0;
            }
            else if (c == CHAR_F3 || c == 27) {
                if (editor.modified) {
                    vga_fill_rect(25, 11, 30, 3, ' ', VGA_WHITE, VGA_RED);
                    vga_draw_box_single(25, 11, 30, 3, VGA_WHITE, VGA_RED);
                    vga_set_color(VGA_WHITE, VGA_RED);
                    vga_print_centered("Unsaved changes! Exit anyway? (Y/N)", 12);
                    vga_swap_buffers();
                    
                    while (1) {
                        keyboard_poll();
                        if (has_key()) {
                            char confirm = get_char();
                            if (confirm == 'y' || confirm == 'Y') {
                                running = 0;
                                break;
                            } else {
                                need_redraw = 1;
                                break;
                            }
                        }
                    }
                } else {
                    running = 0;
                }
            }
            else if (c == '\b') {
                if (editor.buffer_size > 0) {
                    editor.buffer_size--;
                    editor.buffer[editor.buffer_size] = '\0';
                    editor.modified = 1;
                    need_redraw = 1;
                }
            }
            else if (is_printable(c) || c == '\n') {
                if (editor.buffer_size < EDITOR_BUFFER_SIZE - 1) {
                    editor.buffer[editor.buffer_size++] = c;
                    editor.buffer[editor.buffer_size] = '\0';
                    editor.modified = 1;
                    need_redraw = 1;
                }
            }
        }
        
        for (volatile int i = 0; i < 10000; i++) asm volatile("nop");
    }
}

void file_viewer(const char* filename) {
    uint32_t file_size;
    char* content = fat16_read_file(filename, &file_size);
    
    if (!content) {
        vga_println("Error reading file!");
        wait_for_char();
        return;
    }
    
    vga_begin_batch();
    vga_clear();
    
    vga_fill_rect(0, 0, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
    vga_set_color(VGA_WHITE, VGA_BLUE);
    vga_print_centered(filename, 0);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_putchr_at(0, 2, 0);
    
    int row = 2;
    int col = 0;
    
    for (uint32_t i = 0; i < file_size && row < 23; i++) {
        char c = content[i];
        
        if (c == '\n') {
            row++;
            col = 0;
        } else if (c >= 32 && c < 127) {
            vga_putchr_at(col, row, c);
            col++;
            if (col >= 80) {
                col = 0;
                row++;
            }
        }
    }
    
    vga_fill_rect(0, 24, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
    vga_set_color(VGA_YELLOW, VGA_DGREY);
    vga_print_centered("Press any key to return", 24);
    
    vga_end_batch();
    
    wait_for_char();
}

void file_manager_command(void) {
    fat16_init();
    
    vga_clear();
    
    vga_begin_batch();
    vga_fill_rect(0, 0, 80, 2, ' ', VGA_WHITE, VGA_BLUE);
    vga_set_color(VGA_WHITE, VGA_BLUE);
    vga_print_centered("SmolOS File Manager - FAT16", 0);
    vga_print_centered("Type 'help' for commands", 1);
    vga_end_batch();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_putchr_at(0, 3, 0);
    vga_println("");
    
    char input[128];
    
    while (1) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_print("file> ");
        vga_set_color(VGA_WHITE, VGA_BLCK);
        
        int pos = 0;
        while (1) {
            keyboard_poll();
            if (has_key()) {
                char c = get_char();
                
                if (c == '\n') {
                    input[pos] = '\0';
                    vga_putchr('\n');
                    break;
                }
                else if (c == '\b') {
                    if (pos > 0) {
                        pos--;
                        vga_backspace();
                    }
                }
                else if (is_printable(c) && pos < 127) {
                    input[pos++] = c;
                    vga_putchr(c);
                }
            }
        }
        
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            break;
        }
        else if (strcmp(input, "help") == 0) {
            vga_set_color(VGA_LCYAN, VGA_BLCK);
            vga_println("=== File Manager Commands ===");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            vga_println("  list / ls          - List all files");
            vga_println("  view <file>        - View file content");
            vga_println("  edit <file>        - Edit file (F1:Save, F2:Save&Exit, F3:Exit)");
            vga_println("  create <file>      - Create new empty file");
            vga_println("  delete / rm <file> - Delete file");
            vga_println("  info <file>        - Show file information");
            vga_println("  diskinfo           - Show disk space information");
            vga_println("  clear              - Clear screen");
            vga_println("  exit / quit        - Return to shell");
            vga_println("");
        }
        else if (strcmp(input, "list") == 0 || strcmp(input, "ls") == 0) {
            FAT16_FileInfo files[50];
            int count = fat16_list_files(files, 50);
            
            vga_set_color(VGA_LCYAN, VGA_BLCK);
            vga_println("=== Files ===");
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            vga_println("Name                    Size        ");
            vga_println("--------------------------------------");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            
            for (int i = 0; i < count; i++) {
                vga_print(files[i].name);
                
                int name_len = strlen(files[i].name);
                for (int j = name_len; j < 24; j++) vga_putchr(' ');
                
                char size_str[32];
                format_size(files[i].size, size_str);
                vga_println(size_str);
            }
            
            vga_println("");
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_print("Total files: ");
            vga_print_int(count);
            vga_println("");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            vga_println("");
        }
        else if (strstartswith(input, "view ")) {
            char filename[32];
            extract_filename(input, filename);
            
            if (fat16_file_exists(filename)) {
                file_viewer(filename);
                vga_clear();
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_print("Error: File '");
                vga_print(filename);
                vga_println("' not found!");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }
        }
        else if (strstartswith(input, "edit ")) {
            char filename[32];
            extract_filename(input, filename);
            
            if (fat16_file_exists(filename)) {
                text_editor(filename);
                vga_clear();
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_print("Error: File '");
                vga_print(filename);
                vga_println("' not found! Use 'create' first.");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }
        }
        else if (strstartswith(input, "create ")) {
            char filename[32];
            extract_filename(input, filename);
            
            int result = fat16_create_file(filename, "", 0);
            if (result == 0) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
                vga_print("File '");
                vga_print(filename);
                vga_println("' created successfully!");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            } else if (result == -2) {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_println("Error: File already exists!");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_println("Error creating file!");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }
        }
        else if (strstartswith(input, "delete ") || strstartswith(input, "rm ")) {
            char filename[32];
            extract_filename(input, filename);
            
            if (fat16_file_exists(filename)) {
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_print("Delete '");
                vga_print(filename);
                vga_print("'? (y/n): ");
                vga_set_color(VGA_WHITE, VGA_BLCK);
                
                char confirm = wait_for_char();
                vga_putchr(confirm);
                vga_println("");
                
                if (confirm == 'y' || confirm == 'Y') {
                    if (fat16_delete_file(filename) == 0) {
                        vga_set_color(VGA_LGREEN, VGA_BLCK);
                        vga_println("File deleted.");
                        vga_set_color(VGA_WHITE, VGA_BLCK);
                    } else {
                        vga_set_color(VGA_LRED, VGA_BLCK);
                        vga_println("Error deleting file.");
                        vga_set_color(VGA_WHITE, VGA_BLCK);
                    }
                } else {
                    vga_println("Deletion cancelled.");
                }
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_print("Error: File '");
                vga_print(filename);
                vga_println("' not found!");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }
        }
        else if (strstartswith(input, "info ")) {
            char filename[32];
            extract_filename(input, filename);
            
            if (fat16_file_exists(filename)) {
                uint32_t size = fat16_get_file_size(filename);
                
                vga_set_color(VGA_LCYAN, VGA_BLCK);
                vga_println("=== File Information ===");
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_print("Name: ");
                vga_set_color(VGA_WHITE, VGA_BLCK);
                vga_println(filename);
                
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_print("Size: ");
                vga_set_color(VGA_WHITE, VGA_BLCK);
                char size_str[32];
                format_size(size, size_str);
                vga_print(size_str);
                vga_print(" (");
                vga_print_int(size);
                vga_println(" bytes)");
                vga_println("");
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_print("Error: File '");
                vga_print(filename);
                vga_println("' not found!");
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }
        }
        else if (strcmp(input, "diskinfo") == 0) {
            uint32_t total = fat16_get_total_space();
            uint32_t free = fat16_get_free_space();
            uint32_t used = total - free;
            
            vga_set_color(VGA_LCYAN, VGA_BLCK);
            vga_println("=== Disk Information ===");
            
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            vga_print("Total Space: ");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            char size_str[32];
            format_size(total, size_str);
            vga_println(size_str);
            
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            vga_print("Used Space:  ");
            vga_set_color(VGA_WHITE, VGA_BLCK);
            format_size(used, size_str);
            vga_println(size_str);
            
            vga_set_color(VGA_YELLOW, VGA_BLCK);
            vga_print("Free Space:  ");
            vga_set_color(VGA_LGREEN, VGA_BLCK);
            format_size(free, size_str);
            vga_println(size_str);
            
            vga_set_color(VGA_WHITE, VGA_BLCK);
            vga_println("");
        }
        else if (strcmp(input, "clear") == 0 || strcmp(input, "cls") == 0) {
            vga_clear();
        }
        else if (input[0] != '\0') {
            vga_set_color(VGA_LRED, VGA_BLCK);
            vga_print("Unknown command: '");
            vga_print(input);
            vga_println("'");
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_println("Type 'help' for available commands.");
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
    }
    
    vga_clear();
}