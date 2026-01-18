#include "sos_cmdsys.h"
#include "sos_cmds.h"
#include "sos_vga.h"
#include "sos_rtc.h"
#include "sos_mouse.h"
#include "sos_ata.h"
#include "sos_net.h"
#include "sos_mousecalib.h"

void cmd_netinfo_(void){
    cmd_netinfo();
}

void cmd_pitinfo(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== PIT (Programmable Interval Timer) Info ===");
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Frequency:       ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_int(pit_get_frequency());
    vga_println(" Hz");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Tick Interval:   ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    uint32_t interval_us = 1000000 / pit_get_frequency();
    vga_print_int(interval_us);
    vga_println(" microseconds");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Total Ticks:     ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    uint64_t ticks = pit_get_ticks();
    uint32_t high = (uint32_t)(ticks >> 32);
    uint32_t low = (uint32_t)(ticks & 0xFFFFFFFF);
    
    if (high > 0) {
        vga_print_int(high);
        char low_str[12];
        int pos = 10;
        low_str[pos--] = '\0';
        uint32_t temp = low;
        for (int i = 0; i < 10; i++) {
            low_str[pos--] = '0' + (temp % 10);
            temp /= 10;
        }
        vga_print(low_str);
    } else {
        vga_print_int(low);
    }
    vga_println("");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Uptime:          ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    char uptime_str[64];
    pit_format_uptime(uptime_str);
    print_with_scroll(uptime_str);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll("");
}

void cmd_benchmark(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== CPU Benchmark ===");
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    print_with_scroll("Running calculations...");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    pit_start_timer();
    volatile uint32_t result = 0;
    for (uint32_t i = 0; i < 1000000; i++) {
        result += i * 2;
        result -= i / 2;
    }
    uint32_t int_ticks = pit_stop_timer();
    
    vga_print("Integer ops:     ");
    vga_print_int(int_ticks);
    vga_print(" ticks (");
    vga_print_int((int_ticks * 1000) / pit_get_frequency());
    vga_println(" ms)");
    shell_check_scroll();
    
    pit_start_timer();
    volatile uint8_t buffer[1024];
    for (uint32_t i = 0; i < 1000; i++) {
        for (uint32_t j = 0; j < 1024; j++) {
            buffer[j] = (uint8_t)(i + j);
        }
    }
    uint32_t mem_ticks = pit_stop_timer();
    
    vga_print("Memory ops:      ");
    vga_print_int(mem_ticks);
    vga_print(" ticks (");
    vga_print_int((mem_ticks * 1000) / pit_get_frequency());
    vga_println(" ms)");
    shell_check_scroll();
    
    pit_start_timer();
    volatile uint32_t div_result = 123456789;
    for (uint32_t i = 1; i < 10000; i++) {
        div_result = div_result / i;
        div_result = div_result * i;
    }
    uint32_t div_ticks = pit_stop_timer();
    
    vga_print("Division ops:    ");
    vga_print_int(div_ticks);
    vga_print(" ticks (");
    vga_print_int((div_ticks * 1000) / pit_get_frequency());
    vga_println(" ms)");
    shell_check_scroll();
    
    print_with_scroll("");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    print_with_scroll("Benchmark complete!");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_perfmon(CommandArgs args) {
    vga_begin_batch();
    vga_clear();
    
    vga_draw_box_double(5, 2, 70, 20, VGA_CYAN, VGA_BLCK);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("=== Performance Monitor ===", 3);
    vga_draw_separator(6, 4, 68, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Press ESC to exit", 21);
    
    vga_end_batch();
    
    uint64_t last_ticks = pit_get_ticks();
    uint64_t last_rtc_count = rtc_get_interrupt_count();
    
    int frame = 0;
    
    while (1) {
        keyboard_poll();
        
        if (has_key()) {
            char c = get_char();
            if (c == 27) break; 
        }
        
        uint64_t current_ticks = pit_get_ticks();
        uint64_t current_rtc = rtc_get_interrupt_count();
        
        uint64_t pit_rate = current_ticks - last_ticks;
        uint64_t rtc_rate = current_rtc - last_rtc_count;
        
        vga_begin_batch();
        
        vga_fill_rect(8, 6, 64, 10, ' ', VGA_WHITE, VGA_BLCK);
        
        print_at(8, 6, "PIT Timer:", VGA_LCYAN, VGA_BLCK);
        
        print_at(10, 7, "Frequency:   ", VGA_DGREY, VGA_BLCK);
        vga_set_color(VGA_WHITE, VGA_BLCK);
        uint32_t freq = pit_get_frequency();
        char freq_str[16];
        int freq_len = 0;
        if (freq == 0) freq_str[freq_len++] = '0';
        else {
            char temp[16];
            int tlen = 0;
            while (freq > 0) {
                temp[tlen++] = '0' + (freq % 10);
                freq /= 10;
            }
            for (int i = tlen - 1; i >= 0; i--) freq_str[freq_len++] = temp[i];
        }
        freq_str[freq_len] = '\0';
        for (int i = 0; freq_str[i]; i++) vga_putchr_at(23 + i, 7, freq_str[i]);
        print_at(23 + freq_len, 7, " Hz", VGA_WHITE, VGA_BLCK);
        
        print_at(10, 8, "Total Ticks: ", VGA_DGREY, VGA_BLCK);
        vga_set_color(VGA_WHITE, VGA_BLCK);
        uint32_t ticks = (uint32_t)current_ticks;
        char ticks_str[16];
        int ticks_len = 0;
        if (ticks == 0) ticks_str[ticks_len++] = '0';
        else {
            char temp[16];
            int tlen = 0;
            while (ticks > 0) {
                temp[tlen++] = '0' + (ticks % 10);
                ticks /= 10;
            }
            for (int i = tlen - 1; i >= 0; i--) ticks_str[ticks_len++] = temp[i];
        }
        ticks_str[ticks_len] = '\0';
        for (int i = 0; ticks_str[i]; i++) vga_putchr_at(23 + i, 8, ticks_str[i]);
        
        print_at(10, 9, "Uptime:      ", VGA_DGREY, VGA_BLCK);
        char uptime_str[32];
        pit_format_uptime(uptime_str);
        print_at(23, 9, uptime_str, VGA_LGREEN, VGA_BLCK);
        
        print_at(10, 10, "Rate:        ", VGA_DGREY, VGA_BLCK);
        vga_set_color(VGA_WHITE, VGA_BLCK);
        uint32_t rate = (uint32_t)pit_rate;
        char rate_str[16];
        int rate_len = 0;
        if (rate == 0) rate_str[rate_len++] = '0';
        else {
            char temp[16];
            int tlen = 0;
            while (rate > 0) {
                temp[tlen++] = '0' + (rate % 10);
                rate /= 10;
            }
            for (int i = tlen - 1; i >= 0; i--) rate_str[rate_len++] = temp[i];
        }
        rate_str[rate_len] = '\0';
        for (int i = 0; rate_str[i]; i++) vga_putchr_at(23 + i, 10, rate_str[i]);
        print_at(23 + rate_len, 10, " ticks/sec", VGA_WHITE, VGA_BLCK);
        
        print_at(8, 12, "RTC Timer:", VGA_LCYAN, VGA_BLCK);
        
        print_at(10, 13, "Interrupts:  ", VGA_DGREY, VGA_BLCK);
        vga_set_color(VGA_WHITE, VGA_BLCK);
        uint32_t rtc_int = (uint32_t)current_rtc;
        char rtc_str[16];
        int rtc_len = 0;
        if (rtc_int == 0) rtc_str[rtc_len++] = '0';
        else {
            char temp[16];
            int tlen = 0;
            while (rtc_int > 0) {
                temp[tlen++] = '0' + (rtc_int % 10);
                rtc_int /= 10;
            }
            for (int i = tlen - 1; i >= 0; i--) rtc_str[rtc_len++] = temp[i];
        }
        rtc_str[rtc_len] = '\0';
        for (int i = 0; rtc_str[i]; i++) vga_putchr_at(23 + i, 13, rtc_str[i]);
        
        print_at(10, 14, "Rate:        ", VGA_DGREY, VGA_BLCK);
        vga_set_color(VGA_WHITE, VGA_BLCK);
        uint32_t rtc_rate_val = (uint32_t)rtc_rate;
        char rtc_rate_str[16];
        int rtc_rate_len = 0;
        if (rtc_rate_val == 0) rtc_rate_str[rtc_rate_len++] = '0';
        else {
            char temp[16];
            int tlen = 0;
            while (rtc_rate_val > 0) {
                temp[tlen++] = '0' + (rtc_rate_val % 10);
                rtc_rate_val /= 10;
            }
            for (int i = tlen - 1; i >= 0; i--) rtc_rate_str[rtc_rate_len++] = temp[i];
        }
        rtc_rate_str[rtc_rate_len] = '\0';
        for (int i = 0; rtc_rate_str[i]; i++) vga_putchr_at(23 + i, 14, rtc_rate_str[i]);
        print_at(23 + rtc_rate_len, 14, " ticks/sec", VGA_WHITE, VGA_BLCK);
        
        print_at(10, 15, "Time:        ", VGA_DGREY, VGA_BLCK);
        RTCTime time;
        rtc_get_local_time(&time);
        char time_str[16];
        rtc_format_time(&time, time_str);
        print_at(23, 15, time_str, VGA_YELLOW, VGA_BLCK);
        
        char spinner[] = {'|', '/', '-', '\\'};
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_putchr_at(70, 3, spinner[frame % 4]);
        
        vga_end_batch();
        
        frame++;
        last_ticks = current_ticks;
        last_rtc_count = current_rtc;
        
        pit_delay_ms(100);
    }
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_hdinfo(CommandArgs args) {
    ata_init();
    
    if (!ata_is_available()) {
        shell_print_error("No ATA disk detected!");
        vga_set_color(VGA_DGREY, VGA_BLCK);
        print_with_scroll("The system is using RAM-based virtual disk.");
        print_with_scroll("To use a real disk, ensure QEMU has a disk image attached:");
        print_with_scroll("  qemu-system-i386 -kernel SmolOS.bin -hda disk.img");
        vga_set_color(VGA_WHITE, VGA_BLCK);
        return;
    }
    
    ATA_IdentifyInfo info;
    if (ata_identify(&info) != 0) {
        shell_print_error("Failed to identify disk!");
        return;
    }
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== ATA Hard Disk Information ===");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Model:         ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll(info.model);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Serial:        ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll(info.serial);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Total Sectors: ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_int(info.lba28_sectors);
    vga_println("");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Capacity:      ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    uint32_t size_mb = (info.lba28_sectors / 2048);  
    vga_print_int(size_mb);
    vga_println(" MB");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("LBA Support:   ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll(info.supports_lba ? "Yes" : "No");
    
    print_with_scroll("");
    
    if (fat16_using_real_disk()) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        print_with_scroll("[S] FAT16 filesystem is stored on this disk");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    } else {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        print_with_scroll("[F] FAT16 filesystem is in RAM (not persistent)");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    }
}

void cmd_mousetest(CommandArgs args) {
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("=== Mouse Test ===", 1);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("Move the mouse and click buttons", 2);
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print_centered("Press ESC to exit", 23);
    
    vga_draw_box_double(2, 4, 76, 15, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_at(5, 6, "Position:", VGA_YELLOW, VGA_BLCK);
    print_at(5, 7, "Buttons:", VGA_YELLOW, VGA_BLCK);
    print_at(5, 8, "Events:", VGA_YELLOW, VGA_BLCK);
    print_at(5, 10, "Left Button:", VGA_YELLOW, VGA_BLCK);
    print_at(5, 11, "Middle Button:", VGA_YELLOW, VGA_BLCK);
    print_at(5, 12, "Right Button:", VGA_YELLOW, VGA_BLCK);
    
    print_at(5, 14, "Instructions:", VGA_LCYAN, VGA_BLCK);
    print_at(7, 15, "- Move mouse to see coordinates", VGA_LGREY, VGA_BLCK);
    print_at(7, 16, "- Click buttons to test them", VGA_LGREY, VGA_BLCK);
    print_at(7, 17, "- Cursor shown as white block", VGA_LGREY, VGA_BLCK);
    
    vga_end_batch();
    
    mouse_show_cursor();
    int running = 1;
    int event_count = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            if (c == 27) { 
                running = 0;
            }
        }
        

        MouseEvent event;
        while (mouse_get_event(&event)) {
            event_count++;
            

            int x, y;
            mouse_get_position(&x, &y);
            
            vga_begin_batch();
            
            vga_fill_rect(15, 6, 20, 1, ' ', VGA_WHITE, VGA_BLCK);
            print_at(15, 6, "X:", VGA_WHITE, VGA_BLCK);
            char num_buf[10];
            int pos = 0;
            int temp_x = x;
            if (temp_x == 0) {
                num_buf[pos++] = '0';
            } else {
                char rev[10];
                int rev_pos = 0;
                while (temp_x > 0) {
                    rev[rev_pos++] = '0' + (temp_x % 10);
                    temp_x /= 10;
                }
                while (rev_pos > 0) {
                    num_buf[pos++] = rev[--rev_pos];
                }
            }
            num_buf[pos] = '\0';
            print_at(18, 6, num_buf, VGA_YELLOW, VGA_BLCK);
            
            print_at(25, 6, "Y:", VGA_WHITE, VGA_BLCK);
            pos = 0;
            int temp_y = y;
            if (temp_y == 0) {
                num_buf[pos++] = '0';
            } else {
                char rev[10];
                int rev_pos = 0;
                while (temp_y > 0) {
                    rev[rev_pos++] = '0' + (temp_y % 10);
                    temp_y /= 10;
                }
                while (rev_pos > 0) {
                    num_buf[pos++] = rev[--rev_pos];
                }
            }
            num_buf[pos] = '\0';
            print_at(28, 6, num_buf, VGA_YELLOW, VGA_BLCK);
            
            vga_fill_rect(15, 8, 10, 1, ' ', VGA_WHITE, VGA_BLCK);
            pos = 0;
            int temp_count = event_count;
            if (temp_count == 0) {
                num_buf[pos++] = '0';
            } else {
                char rev[10];
                int rev_pos = 0;
                while (temp_count > 0) {
                    rev[rev_pos++] = '0' + (temp_count % 10);
                    temp_count /= 10;
                }
                while (rev_pos > 0) {
                    num_buf[pos++] = rev[--rev_pos];
                }
            }
            num_buf[pos] = '\0';
            print_at(15, 8, num_buf, VGA_YELLOW, VGA_BLCK);
            
            print_at(20, 10, mouse_is_left_pressed() ? "[PRESSED]" : "[........]",
                     mouse_is_left_pressed() ? VGA_LGREEN : VGA_DGREY, VGA_BLCK);
            print_at(20, 11, mouse_is_middle_pressed() ? "[PRESSED]" : "[........]",
                     mouse_is_middle_pressed() ? VGA_LGREEN : VGA_DGREY, VGA_BLCK);
            print_at(20, 12, mouse_is_right_pressed() ? "[PRESSED]" : "[........]",
                     mouse_is_right_pressed() ? VGA_LGREEN : VGA_DGREY, VGA_BLCK);
            
            vga_end_batch();
        }
        
        for (volatile int i = 0; i < 1000; i++) {
            asm volatile("nop");
        }
    }
    
    mouse_hide_cursor();
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_mouse_calibrate(CommandArgs args) {
    vga_clear();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Starting mouse calibration...");
    vga_println("");
    pit_delay_ms(1000);
    
    mouse_run_calibration();
    
    vga_clear();
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("Mouse calibration complete!");
    vga_println("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_mouse_reset_calibration(CommandArgs args) {
    mouse_reset_calibration();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Mouse calibration has been reset.");
    vga_println("Calibration will be required on next mouse use.");
    shell_check_scroll();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_mouse_calibration_info(CommandArgs args) {
    CalibrationData* cal_data = calibration_get_data();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("Mouse Calibration Information:");
    vga_println("=============================");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Status: ");
    
    if (cal_data->is_calibrated) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_println("Calibrated");
    } else {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_println("Not Calibrated");
    }
    shell_check_scroll();
    
    if (cal_data->is_calibrated) {
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_print("Center Position: (");
        vga_print_int(cal_data->center_x);
        vga_print(", ");
        vga_print_int(cal_data->center_y);
        vga_println(")");
        shell_check_scroll();
        
        vga_print("Calibration Time: ");
        vga_print_int(cal_data->calibration_timestamp);
        vga_println(" seconds since boot");
        shell_check_scroll();
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("");
    vga_println("Debug Information:");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    
    vga_print("Calibration File (MOUSECAL.DAT): ");
    if (fat16_file_exists("MOUSECAL.DAT")) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_print("EXISTS");
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_print(" (");
        vga_print_int(fat16_get_file_size("MOUSECAL.DAT"));
        vga_println(" bytes)");
    } else {
        vga_set_color(VGA_LRED, VGA_BLCK);
        vga_println("NOT FOUND");
        vga_set_color(VGA_WHITE, VGA_BLCK);
    }
    shell_check_scroll();
    
    if (fat16_file_exists("MOUSECAL.DAT")) {
        uint32_t file_size;
        char* content = fat16_read_file("MOUSECAL.DAT", &file_size);
        
        if (content && file_size >= 24) {
            typedef struct {
                int magic_number;
                int center_x;
                int center_y;
                int is_calibrated;
                uint32_t calibration_timestamp;
                int checksum;
            } CalibrationFileData;
            
            CalibrationFileData* data = (CalibrationFileData*)content;
            
            vga_print("Magic Number: 0x");
            vga_print_hex(data->magic_number);
            vga_print(" (Expected: 0x4D43414C)");
            if (data->magic_number == 0x4D43414C) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
                vga_println(" [Y]");
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_println(" [N]");
            }
            vga_set_color(VGA_WHITE, VGA_BLCK);
            shell_check_scroll();
            
            vga_print("Stored Position: (");
            vga_print_int(data->center_x);
            vga_print(", ");
            vga_print_int(data->center_y);
            vga_println(")");
            shell_check_scroll();
            
            vga_print("Stored Calibrated Flag: ");
            vga_print_int(data->is_calibrated);
            vga_println("");
            shell_check_scroll();
            
            int calc_checksum = data->magic_number + data->center_x + 
                               data->center_y + data->is_calibrated + 
                               (int)data->calibration_timestamp;
            
            vga_print("Checksum: ");
            vga_print_int(data->checksum);
            vga_print(" (Calculated: ");
            vga_print_int(calc_checksum);
            vga_print(")");
            
            if (calc_checksum == data->checksum) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
                vga_println(" [Y]");
            } else {
                vga_set_color(VGA_LRED, VGA_BLCK);
                vga_println(" [N] MISMATCH!");
            }
            vga_set_color(VGA_WHITE, VGA_BLCK);
            shell_check_scroll();
        }
    }
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_println("");
    vga_println("Use 'mousecalibrate' to recalibrate");
    shell_check_scroll();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_mousedraw(CommandArgs args) {
    vga_begin_batch();
    vga_clear();
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("=== Mouse Drawing Program ===", 1);
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print_centered("Left: Draw | Right: Erase | C: Clear | ESC: Exit", 2);
    
    vga_draw_box_single(0, 3, 80, 19, VGA_CYAN, VGA_BLCK);
    
    vga_end_batch();
    
    mouse_show_cursor();
    mouse_set_cursor_char('+');
    
    int running = 1;
    int last_x = -1, last_y = -1;
    int drawing = 0;
    
    while (running) {
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            if (c == 27) { 
                running = 0;
            } else if (c == 'c' || c == 'C') {
                vga_begin_batch();
                vga_fill_rect(1, 4, 78, 17, ' ', VGA_WHITE, VGA_BLCK);
                vga_end_batch();
            }
        }
        
        int mx, my;
        mouse_get_position(&mx, &my);
        
        int inside = (mx >= 1 && mx <= 78 && my >= 4 && my <= 21);
        
        if (inside) {
            if (mouse_is_left_pressed()) {
                vga_putchr_direct(mx, my, 0xDB, 0x0F);  
                
                if (drawing && last_x >= 0 && last_y >= 0 && 
                    (last_x != mx || last_y != my)) {
                    int dx = mx - last_x;
                    int dy = my - last_y;
                    int steps = (dx < 0 ? -dx : dx);
                    int ysteps = (dy < 0 ? -dy : dy);
                    if (ysteps > steps) steps = ysteps;
                    
                    if (steps > 0) {
                        for (int i = 0; i <= steps; i++) {
                            int ix = last_x + (dx * i) / steps;
                            int iy = last_y + (dy * i) / steps;
                            if (ix >= 1 && ix <= 78 && iy >= 4 && iy <= 21) {
                                vga_putchr_direct(ix, iy, 0xDB, 0x0F);
                            }
                        }
                    }
                }
                drawing = 1;
                last_x = mx;
                last_y = my;
            } else if (mouse_is_right_pressed()) {
                vga_putchr_direct(mx, my, ' ', 0x07);
                
                if (drawing && last_x >= 0 && last_y >= 0 && 
                    (last_x != mx || last_y != my)) {
                    int dx = mx - last_x;
                    int dy = my - last_y;
                    int steps = (dx < 0 ? -dx : dx);
                    int ysteps = (dy < 0 ? -dy : dy);
                    if (ysteps > steps) steps = ysteps;
                    
                    if (steps > 0) {
                        for (int i = 0; i <= steps; i++) {
                            int ix = last_x + (dx * i) / steps;
                            int iy = last_y + (dy * i) / steps;
                            if (ix >= 1 && ix <= 78 && iy >= 4 && iy <= 21) {
                                vga_putchr_direct(ix, iy, ' ', 0x07);
                            }
                        }
                    }
                }
                drawing = 1;
                last_x = mx;
                last_y = my;
            } else {
                drawing = 0;
                last_x = -1;
                last_y = -1;
            }
        } else {
            drawing = 0;
            last_x = -1;
            last_y = -1;
        }
        
        pit_delay_ms(5); 
    }
    
    mouse_hide_cursor();
    mouse_set_cursor_char(0xDB);
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void cmd_mouseinfo(CommandArgs args) {
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== Mouse Information ===");
    print_with_scroll("");
    
    MouseState state;
    mouse_get_state(&state);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Status: ");
    vga_set_color(state.initialized ? VGA_LGREEN : VGA_LRED, VGA_BLCK);
    print_with_scroll(state.initialized ? "Initialized" : "Not initialized");
    
    if (!state.initialized) {
        vga_set_color(VGA_WHITE, VGA_BLCK);
        return;
    }
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Position: ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("X=");
    vga_print_int(state.x);
    vga_print(", Y=");
    vga_print_int(state.y);
    vga_println("");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Buttons: ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Left=");
    vga_print(mouse_is_left_pressed() ? "DOWN" : "UP");
    vga_print(", Middle=");
    vga_print(mouse_is_middle_pressed() ? "DOWN" : "UP");
    vga_print(", Right=");
    vga_print(mouse_is_right_pressed() ? "DOWN" : "UP");
    vga_println("");
    shell_check_scroll();
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Cursor: ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll(state.visible ? "Visible" : "Hidden");
    
    uint32_t int_count = mouse_get_interrupt_count();
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Interrupts: ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print_int(int_count);
    vga_println("");
    shell_check_scroll();
    
    if (int_count == 0) {
        vga_set_color(VGA_LRED, VGA_BLCK);
        print_with_scroll("");
        print_with_scroll("WARNING: No mouse interrupts received!");
        print_with_scroll("Check that IRQ12 is enabled.");
    }
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll("");
}

void cmd_diskinfo_enhanced(CommandArgs args) {
    fat16_init();
    
    uint32_t total = fat16_get_total_space();
    uint32_t free = fat16_get_free_space();
    uint32_t used = total - free;
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    print_with_scroll("=== File System Information ===");
    print_with_scroll("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Type:        ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll("FAT16");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Storage:     ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll(fat16_using_real_disk() ? "ATA Hard Disk" : "RAM (Virtual)");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Total Space: ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    char size_str[32];
    format_size(total, size_str);
    print_with_scroll(size_str);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Used Space:  ");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    format_size(used, size_str);
    print_with_scroll(size_str);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Free Space:  ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    format_size(free, size_str);
    print_with_scroll(size_str);
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    print_with_scroll("");
    
    int percent = (used * 100) / total;
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_print("Usage:       ");
    vga_set_color(percent > 80 ? VGA_RED : VGA_WHITE, VGA_BLCK);
    vga_print_int(percent);
    vga_println("%");
    shell_check_scroll();
    
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_print("             [");
    int bar_width = 40;
    int filled = (bar_width * percent) / 100;
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            vga_set_color(VGA_LGREEN, VGA_BLCK);
            vga_putchr('#');
        } else {
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_putchr('-');
        }
    }
    vga_set_color(VGA_DGREY, VGA_BLCK);
    vga_println("]");
    shell_check_scroll();
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
}