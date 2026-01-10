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

void text_editor(const char* filename) {
    memset(&editor, 0, sizeof(EditorState));
    strcpy(editor.filename, (char*)filename);
    
    uint32_t file_size;
    char* content = fat16_read_file(filename, &file_size);
    
    if (content && file_size > 0) {
        int copy_size = (file_size < EDITOR_BUFFER_SIZE - 1) ? file_size : EDITOR_BUFFER_SIZE - 1;
        memcpy(editor.buffer, content, copy_size);
        editor.buffer_size = copy_size;
        editor.buffer[editor.buffer_size] = '\0';
    }
    
    editor.cursor_row = 0;
    editor.cursor_col = 0;
    editor.scroll_offset = 0;
    editor.modified = 0;
    
    int running = 1;
    int need_full_redraw = 1;
    
    while (running) {
        if (need_full_redraw) {
            vga_begin_batch();
            vga_clear();
            
            vga_fill_rect(0, 0, 80, 1, ' ', VGA_BLCK, VGA_CYAN);
            vga_set_color(VGA_WHITE, VGA_CYAN);
            vga_print_centered("SmolOS Text Editor", 0);
            
            vga_fill_rect(0, 1, 80, 1, ' ', VGA_WHITE, VGA_BLUE);
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_putchr_at(2, 1, 0);
            vga_print(editor.filename);
            
            if (editor.modified) {
                vga_set_color(VGA_LRED, VGA_BLUE);
                vga_print(" [Modified]");
            }
            
            vga_set_color(VGA_DGREY, VGA_BLUE);
            char size_str[32];
            format_size(editor.buffer_size, size_str);
            vga_putchr_at(65, 1, 0);
            vga_print(size_str);
            
            vga_set_color(VGA_DGREY, VGA_BLCK);
            for (int x = 0; x < 80; x++) {
                vga_putchr_at(x, 2, 0xC4);  
                vga_putchr_at(x, 22, 0xC4); 
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
            int line_start_col = 0;
            
            while (display_row < 22) {
                vga_set_color(VGA_DGREY, VGA_BLCK);
                int line_num = current_line + 1;
                vga_putchr_at(0, display_row, ' ');
                
                if (line_num >= 100) {
                    vga_putchr_at(1, display_row, '0' + (line_num / 100));
                    vga_putchr_at(2, display_row, '0' + ((line_num / 10) % 10));
                    vga_putchr_at(3, display_row, '0' + (line_num % 10));
                } else if (line_num >= 10) {
                    vga_putchr_at(1, display_row, ' ');
                    vga_putchr_at(2, display_row, '0' + (line_num / 10));
                    vga_putchr_at(3, display_row, '0' + (line_num % 10));
                } else {
                    vga_putchr_at(1, display_row, ' ');
                    vga_putchr_at(2, display_row, ' ');
                    vga_putchr_at(3, display_row, '0' + line_num);
                }
                
                vga_putchr_at(4, display_row, 0xB3); 
                
                vga_set_color(VGA_WHITE, VGA_BLCK);
                line_start_col = 6;
                current_col = line_start_col;
                
                while (char_idx < editor.buffer_size && current_col < 80) {
                    char c = editor.buffer[char_idx];
                    
                    if (c == '\n') {
                        char_idx++;
                        current_line++;
                        break;
                    } else if (c == '\t') {
                        for (int t = 0; t < 4 && current_col < 80; t++) {
                            vga_putchr_at(current_col++, display_row, ' ');
                        }
                        char_idx++;
                    } else if (c >= 32 && c < 127) {
                        vga_putchr_at(current_col++, display_row, c);
                        char_idx++;
                    } else {
                        char_idx++;
                    }
                }
                
                display_row++;
                
                if (char_idx >= editor.buffer_size) {
                    while (display_row < 22) {
                        vga_set_color(VGA_DGREY, VGA_BLCK);
                        vga_putchr_at(0, display_row, '~');
                        vga_putchr_at(4, display_row, 0xB3);
                        display_row++;
                    }
                    break;
                }
            }
            
            int cursor_screen_row = 3 + (editor.cursor_row - editor.scroll_offset);
            int cursor_screen_col = 6 + editor.cursor_col;
            
            if (cursor_screen_row >= 3 && cursor_screen_row < 22 && cursor_screen_col < 80) {
                vga_set_color(VGA_BLCK, VGA_LCYAN);
                char cursor_char = ' ';
                
                int idx = 0;
                int row = 0;
                int col = 0;
                
                while (idx < editor.buffer_size) {
                    if (row == editor.cursor_row && col == editor.cursor_col) {
                        if (editor.buffer[idx] >= 32 && editor.buffer[idx] < 127) {
                            cursor_char = editor.buffer[idx];
                        }
                        break;
                    }
                    
                    if (editor.buffer[idx] == '\n') {
                        if (row == editor.cursor_row) break;
                        row++;
                        col = 0;
                    } else if (editor.buffer[idx] == '\t') {
                        col += 4;
                    } else {
                        col++;
                    }
                    idx++;
                }
                
                vga_putchr_at(cursor_screen_col, cursor_screen_row, cursor_char);
            }
            
            vga_fill_rect(0, 23, 80, 1, ' ', VGA_WHITE, VGA_DGREY);
            vga_set_color(VGA_YELLOW, VGA_DGREY);
            vga_putchr_at(1, 23, 0);
            vga_print("F1");
            vga_set_color(VGA_WHITE, VGA_DGREY);
            vga_print(":Save ");
            
            vga_set_color(VGA_YELLOW, VGA_DGREY);
            vga_print("F2");
            vga_set_color(VGA_WHITE, VGA_DGREY);
            vga_print(":Save&Exit ");
            
            vga_set_color(VGA_YELLOW, VGA_DGREY);
            vga_print("F3");
            vga_set_color(VGA_WHITE, VGA_DGREY);
            vga_print(":Exit ");
            
            vga_set_color(VGA_YELLOW, VGA_DGREY);
            vga_print("ESC");
            vga_set_color(VGA_WHITE, VGA_DGREY);
            vga_print(":Quit");
            
            vga_fill_rect(0, 24, 80, 1, ' ', VGA_BLCK, VGA_LGREY);
            vga_set_color(VGA_BLCK, VGA_LGREY);
            
            vga_putchr_at(2, 24, 0);
            vga_print("Line:");
            vga_print_int(editor.cursor_row + 1);
            
            vga_putchr_at(15, 24, 0);
            vga_print("Col:");
            vga_print_int(editor.cursor_col + 1);
            
            vga_putchr_at(27, 24, 0);
            vga_print("Size:");
            format_size(editor.buffer_size, size_str);
            vga_print(size_str);
            
            vga_putchr_at(45, 24, 0);
            vga_print("Chars:");
            vga_print_int(editor.buffer_size);
            
            int total_lines = 1;
            for (int i = 0; i < editor.buffer_size; i++) {
                if (editor.buffer[i] == '\n') total_lines++;
            }
            vga_putchr_at(62, 24, 0);
            vga_print("Lines:");
            vga_print_int(total_lines);
            
            vga_end_batch();
            need_full_redraw = 0;
        }
        
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            
            if (c == CHAR_F1) {
                fat16_write_file(editor.filename, editor.buffer, editor.buffer_size);
                editor.modified = 0;
                need_full_redraw = 1;
            }
            else if (c == CHAR_F2) {
                fat16_write_file(editor.filename, editor.buffer, editor.buffer_size);
                running = 0;
            }
            else if (c == CHAR_F3 || c == 27) {
                if (editor.modified) {
                    vga_begin_batch();
                    vga_fill_rect(20, 10, 40, 5, ' ', VGA_WHITE, VGA_RED);
                    vga_draw_box_single(20, 10, 40, 5, VGA_YELLOW, VGA_RED);
                    vga_set_color(VGA_WHITE, VGA_RED);
                    vga_print_centered("Unsaved Changes!", 11);
                    vga_print_centered("Exit anyway? (Y/N)", 12);
                    vga_end_batch();
                    
                    while (1) {
                        keyboard_poll();
                        if (has_key()) {
                            char confirm = get_char();
                            if (confirm == 'y' || confirm == 'Y') {
                                running = 0;
                                break;
                            } else if (confirm == 'n' || confirm == 'N' || confirm == 27) {
                                need_full_redraw = 1;
                                break;
                            }
                        }
                    }
                    vga_set_color(VGA_WHITE,VGA_BLCK);
                } else {
                    running = 0;
                }
            }
            else if (c == '\b') {
                if (editor.buffer_size > 0) {
                    int idx = 0;
                    int row = 0;
                    int col = 0;
                    
                    while (idx < editor.buffer_size) {
                        if (row == editor.cursor_row && col == editor.cursor_col) {
                            break;
                        }
                        
                        if (editor.buffer[idx] == '\n') {
                            row++;
                            col = 0;
                        } else if (editor.buffer[idx] == '\t') {
                            col += 4;
                        } else {
                            col++;
                        }
                        idx++;
                    }
                    
                    if (idx > 0) {
                        for (int i = idx - 1; i < editor.buffer_size; i++) {
                            editor.buffer[i] = editor.buffer[i + 1];
                        }
                        editor.buffer_size--;
                        editor.modified = 1;
                        
                        if (editor.cursor_col > 0) {
                            editor.cursor_col--;
                        } else if (editor.cursor_row > 0) {
                            editor.cursor_row--;
                            int line_len = 0;
                            int search_idx = 0;
                            int search_row = 0;
                            
                            while (search_idx < editor.buffer_size && search_row < editor.cursor_row) {
                                if (editor.buffer[search_idx] == '\n') {
                                    search_row++;
                                    line_len = 0;
                                } else if (editor.buffer[search_idx] == '\t') {
                                    line_len += 4;
                                } else {
                                    line_len++;
                                }
                                search_idx++;
                            }
                            
                            while (search_idx < editor.buffer_size && editor.buffer[search_idx] != '\n') {
                                if (editor.buffer[search_idx] == '\t') {
                                    line_len += 4;
                                } else {
                                    line_len++;
                                }
                                search_idx++;
                            }
                            
                            editor.cursor_col = line_len;
                        }
                        
                        need_full_redraw = 1;
                    }
                }
            }
            else if (c == CHAR_UP) {
                if (editor.cursor_row > 0) {
                    editor.cursor_row--;
                    if (editor.cursor_row < editor.scroll_offset) {
                        editor.scroll_offset = editor.cursor_row;
                        need_full_redraw = 1;
                    } else {
                        need_full_redraw = 1; 
                    }
                }
            }
            else if (c == CHAR_DOWN) {
                int total_lines = 1;
                for (int i = 0; i < editor.buffer_size; i++) {
                    if (editor.buffer[i] == '\n') total_lines++;
                }
                
                if (editor.cursor_row < total_lines - 1) {
                    editor.cursor_row++;
                    if (editor.cursor_row >= editor.scroll_offset + 19) {
                        editor.scroll_offset++;
                        need_full_redraw = 1;
                    } else {
                        need_full_redraw = 1;
                    }
                }
            }
            else if (c == CHAR_LEFT) {
                if (editor.cursor_col > 0) {
                    editor.cursor_col--;
                    need_full_redraw = 1;
                }
            }
            else if (c == CHAR_RIGHT) {
                int idx = 0;
                int row = 0;
                int col = 0;
                int line_len = 0;
                
                while (idx < editor.buffer_size) {
                    if (row == editor.cursor_row) {
                        if (editor.buffer[idx] == '\n') {
                            line_len = col;
                            break;
                        } else if (editor.buffer[idx] == '\t') {
                            col += 4;
                        } else {
                            col++;
                        }
                    }
                    
                    if (editor.buffer[idx] == '\n') {
                        row++;
                        col = 0;
                    }
                    idx++;
                }
                
                if (idx >= editor.buffer_size) line_len = col;
                
                if (editor.cursor_col < line_len) {
                    editor.cursor_col++;
                    need_full_redraw = 1;
                }
            }
            else if (c == CHAR_HOME) {
                editor.cursor_col = 0;
                need_full_redraw = 1;
            }
            else if (c == CHAR_END) {
                int idx = 0;
                int row = 0;
                int col = 0;
                
                while (idx < editor.buffer_size) {
                    if (row == editor.cursor_row) {
                        if (editor.buffer[idx] == '\n') {
                            break;
                        } else if (editor.buffer[idx] == '\t') {
                            col += 4;
                        } else {
                            col++;
                        }
                    }
                    
                    if (editor.buffer[idx] == '\n') {
                        row++;
                        col = 0;
                    }
                    idx++;
                }
                
                editor.cursor_col = col;
                need_full_redraw = 1;
            }
            else if (c == '\n' || (is_printable(c) && c != '\t')) {
                if (editor.buffer_size < EDITOR_BUFFER_SIZE - 1) {
                    int idx = 0;
                    int row = 0;
                    int col = 0;
                    
                    while (idx < editor.buffer_size) {
                        if (row == editor.cursor_row && col == editor.cursor_col) {
                            break;
                        }
                        
                        if (editor.buffer[idx] == '\n') {
                            row++;
                            col = 0;
                        } else if (editor.buffer[idx] == '\t') {
                            col += 4;
                        } else {
                            col++;
                        }
                        idx++;
                    }
                    
                    for (int i = editor.buffer_size; i > idx; i--) {
                        editor.buffer[i] = editor.buffer[i - 1];
                    }
                    
                    editor.buffer[idx] = c;
                    editor.buffer_size++;
                    editor.buffer[editor.buffer_size] = '\0';
                    editor.modified = 1;
                    
                    if (c == '\n') {
                        editor.cursor_row++;
                        editor.cursor_col = 0;
                        
                        if (editor.cursor_row >= editor.scroll_offset + 19) {
                            editor.scroll_offset++;
                        }
                    } else {
                        editor.cursor_col++;
                    }
                    
                    need_full_redraw = 1;
                }
            }
            else if (c == '\t') {
                for (int t = 0; t < 4 && editor.buffer_size < EDITOR_BUFFER_SIZE - 1; t++) {
                    int idx = 0;
                    int row = 0;
                    int col = 0;
                    
                    while (idx < editor.buffer_size) {
                        if (row == editor.cursor_row && col == editor.cursor_col) {
                            break;
                        }
                        
                        if (editor.buffer[idx] == '\n') {
                            row++;
                            col = 0;
                        } else {
                            col++;
                        }
                        idx++;
                    }
                    
                    for (int i = editor.buffer_size; i > idx; i--) {
                        editor.buffer[i] = editor.buffer[i - 1];
                    }
                    
                    editor.buffer[idx] = ' ';
                    editor.buffer_size++;
                    editor.cursor_col++;
                }
                
                editor.buffer[editor.buffer_size] = '\0';
                editor.modified = 1;
                need_full_redraw = 1;
            }
        }
        
        for (volatile int i = 0; i < 5000; i++) asm volatile("nop");
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