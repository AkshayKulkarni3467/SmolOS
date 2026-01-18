#include "sos_cmdfiles.h"
#include "sos_cmds.h"
#include "sos_fat16.h"
#include "sos_vga.h"

void cmd_ls(CommandArgs args) {
    fat16_init();
    
    FAT16_FileInfo files[50];
    int count = fat16_list_files(files, 50);
    
    if (count == 0) {
        print_with_scroll("No files found.");
        return;
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("Files:");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    for (int i = 0; i < count; i++) {
        vga_print("  ");
        vga_print(files[i].name);
        
        int name_len = strlen(files[i].name);
        for (int j = name_len; j < 20; j++) vga_putchr(' ');
        
        char size_str[32];
        format_size(files[i].size, size_str);
        vga_println(size_str);
        shell_check_scroll();
    }
    
    vga_println("");
    shell_check_scroll();
}

void cmd_cat(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: cat <filename>");
        return;
    }
    
    fat16_init();
    
    char* filename = args.args[1];
    
    if (!fat16_file_exists(filename)) {
        shell_print_error("File not found!");
        return;
    }
    
    uint32_t size;
    char* content = fat16_read_file(filename, &size);
    
    if (!content) {
        shell_print_error("Error reading file!");
        return;
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    for (uint32_t i = 0; i < size; i++) {
        vga_putchr(content[i]);
        if (content[i] == '\n') {
            shell_check_scroll();
        }
    }
    
    if (size > 0 && content[size - 1] != '\n') {
        vga_println("");
    }
    
    shell_check_scroll();
}

void cmd_touch(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: touch <filename>");
        return;
    }
    
    fat16_init();
    
    char* filename = args.args[1];
    
    if (fat16_file_exists(filename)) {
        shell_print_info("File already exists.");
        return;
    }
    
    int result = fat16_create_file(filename, "", 0);
    if (result == 0) {
        shell_print_success("File created!");
    } else {
        shell_print_error("Error creating file!");
    }
}

void cmd_rm(CommandArgs args) {
    if (args.argc < 2) {
        shell_print_error("Usage: rm <filename>");
        return;
    }
    
    fat16_init();
    
    char* filename = args.args[1];
    
    if (!fat16_file_exists(filename)) {
        shell_print_error("File not found!");
        return;
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Delete '");
    vga_print(filename);
    vga_print("'? (y/n): ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    char confirm = wait_for_char();
    vga_putchr(confirm);
    vga_println("");
    shell_check_scroll();
    
    if (confirm == 'y' || confirm == 'Y') {
        if (fat16_delete_file(filename) == 0) {
            shell_print_success("File deleted.");
        } else {
            shell_print_error("Error deleting file.");
        }
    } else {
        shell_print_info("Deletion cancelled.");
    }
}