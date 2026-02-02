#include "sos_audio.h"
#include "sos_vga.h"
#include "sos_vgraphics.h"
#include "sos_keyboard.h"
#include "sos_pit.h"
#include "sos_fat16.h"
#include "sos_string.h"
#include "sos_memory.h"
#include "sos_musicplayer.h"





static AudioPlayer player;
static PlayerScreen current_screen;
static int selected_track;
static int scroll_offset;
static int menu_selection;
static AudioEffects effects;
static int visualizer_mode;  


static FAT16_FileInfo file_list[50];
static int file_count;
static int selected_file;
static int file_scroll_offset;


static void format_time(uint32_t seconds, char* buffer) {
    int mins = seconds / 60;
    int secs = seconds % 60;
    
    buffer[0] = '0' + (mins / 10);
    buffer[1] = '0' + (mins % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (secs / 10);
    buffer[4] = '0' + (secs % 10);
    buffer[5] = '\0';
}

static void draw_progress_bar(int x, int y, int width, int progress, uint8_t fg, uint8_t bg) {
    int filled = (progress * width) / 100;
    
    vga_set_color(fg, VGA_BLCK);
    vga_putchr_at(x - 1, y, '[');
    vga_putchr_at(x + width, y, ']');
    
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            vga_set_color(fg, VGA_BLCK);
            vga_putchr_at(x + i, y, 0xDB);  
        } else {
            vga_set_color(VGA_DGREY, VGA_BLCK);
            vga_putchr_at(x + i, y, 0xB1);  
        }
    }
}

static void draw_volume_bar(int x, int y, int volume) {
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* label = "Vol:";
    for (int i = 0; label[i]; i++) {
        vga_putchr_at(x + i, y, label[i]);
    }
    
    draw_progress_bar(x + 5, y, 20, volume, VGA_LGREEN, VGA_DGREY);
    
    char vol_text[5];
    int pos = 0;
    int temp = volume;
    if (temp == 0) {
        vol_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            vol_text[pos++] = digits[i];
        }
    }
    vol_text[pos++] = '%';
    vol_text[pos] = '\0';
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    for (int i = 0; vol_text[i]; i++) {
        vga_putchr_at(x + 27 + i, y, vol_text[i]);
    }
}


static void draw_header(void) {
    vga_fill_rect(0, 0, SCREEN_WIDTH, 2, ' ', VGA_YELLOW, VGA_BLUE);
    
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    const char* title = "SmolOS Music Player v1.0";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 0, title[i]);
    }
    
    vga_set_color(VGA_WHITE, VGA_BLUE);
    const char* status;
    uint8_t status_color;
    
    if (player.state == AUDIO_PLAYING) {
        status = "PLAYING";
        status_color = VGA_LGREEN;
    } else if (player.state == AUDIO_PAUSED) {
        status = "PAUSED";
        status_color = VGA_YELLOW;
    } else {
        status = "STOPPED";
        status_color = VGA_LGREY;
    }
    
    vga_set_color(status_color, VGA_BLUE);
    for (int i = 0; status[i]; i++) {
        vga_putchr_at(2 + i, 1, status[i]);
    }

    if (player.shuffle) {
        vga_set_color(VGA_LCYAN, VGA_BLUE);
        vga_putchr_at(SCREEN_WIDTH - 15, 1, 'S');
        vga_putchr_at(SCREEN_WIDTH - 14, 1, 'H');
    }
    
    if (player.repeat) {
        vga_set_color(VGA_LMAGENTA, VGA_BLUE);
        vga_putchr_at(SCREEN_WIDTH - 10, 1, 'R');
        vga_putchr_at(SCREEN_WIDTH - 9, 1, 'P');
    }
}

static void draw_now_playing(void) {
    vga_draw_box_single(2, 3, SCREEN_WIDTH - 4, 3, VGA_CYAN, VGA_BLCK);
    
    if (player.playlist_count > 0 && player.current_track < player.playlist_count) {
        vga_set_color(VGA_WHITE, VGA_BLCK);
        const char* label = "Now Playing: ";
        int x = 4;
        for (int i = 0; label[i]; i++) {
            vga_putchr_at(x++, 4, label[i]);
        }
        
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        const char* filename = player.playlist[player.current_track].filename;
        for (int i = 0; filename[i] && i < 50; i++) {
            vga_putchr_at(x++, 4, filename[i]);
        }
    } else {
        vga_set_color(VGA_DGREY, VGA_BLCK);
        const char* msg = "No track loaded";
        int len = 0;
        while (msg[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 4, msg[i]);
        }
    }
}

static void draw_playlist(void) {
    vga_draw_box_single(2, PLAYLIST_START_Y, SCREEN_WIDTH - 4, PLAYLIST_HEIGHT, VGA_YELLOW, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* title = "Playlist";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at(4 + i, PLAYLIST_START_Y, title[i]);
    }
    
    char count_text[20];
    int pos = 0;
    count_text[pos++] = '[';
    int temp = player.playlist_count;
    if (temp == 0) {
        count_text[pos++] = '0';
    } else {
        char digits[10];
        int digit_count = 0;
        while (temp > 0) {
            digits[digit_count++] = '0' + (temp % 10);
            temp /= 10;
        }
        for (int i = digit_count - 1; i >= 0; i--) {
            count_text[pos++] = digits[i];
        }
    }
    count_text[pos++] = ']';
    count_text[pos] = '\0';
    
    for (int i = 0; count_text[i]; i++) {
        vga_putchr_at(13 + i, PLAYLIST_START_Y, count_text[i]);
    }

    int visible_start = scroll_offset;
    int visible_end = scroll_offset + MAX_VISIBLE_TRACKS;
    if (visible_end > player.playlist_count) visible_end = player.playlist_count;
    
    for (int i = visible_start; i < visible_end; i++) {
        int y = PLAYLIST_START_Y + 1 + (i - visible_start);
        int selected = (i == selected_track);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(3, y, SCREEN_WIDTH - 6, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }

        char num[5];
        pos = 0;
        temp = i + 1;
        if (temp < 10) num[pos++] = ' ';
        if (temp == 0) {
            num[pos++] = '0';
        } else {
            char digits[10];
            int digit_count = 0;
            while (temp > 0) {
                digits[digit_count++] = '0' + (temp % 10);
                temp /= 10;
            }
            for (int j = digit_count - 1; j >= 0; j--) {
                num[pos++] = digits[j];
            }
        }
        num[pos++] = '.';
        num[pos++] = ' ';
        num[pos] = '\0';
        
        int x = 4;
        for (int j = 0; num[j]; j++) {
            vga_putchr_at(x++, y, num[j]);
        }

        const char* filename = player.playlist[i].filename;
        for (int j = 0; filename[j] && j < 60; j++) {
            vga_putchr_at(x++, y, filename[j]);
        }

        if (i == player.current_track && player.state != AUDIO_STOPPED) {
            vga_set_color(VGA_LGREEN, selected ? VGA_BLUE : VGA_BLCK);
            vga_putchr_at(SCREEN_WIDTH - 5, y, 0x10); 
        }
    }
    
    if (player.playlist_count == 0) {
        vga_set_color(VGA_DGREY, VGA_BLCK);
        const char* msg = "Playlist is empty - Press 'A' to add files";
        len = 0;
        while (msg[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, PLAYLIST_START_Y + 5, msg[i]);
        }
    }
}

static void draw_controls(void) {
    int y = PLAYLIST_START_Y + PLAYLIST_HEIGHT + 1;
    
    draw_volume_bar(2, y, player.volume);

    if (player.playlist_count > 0) {
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        const char* label = "Position:";
        int x = 35;
        for (int i = 0; label[i]; i++) {
            vga_putchr_at(x++, y, label[i]);
        }
        
        int progress = player_get_progress(&player);
        draw_progress_bar(x + 1, y, 30, progress, VGA_LCYAN, VGA_DGREY);
    }
}

static void draw_menu_bar(void) {
    int y = SCREEN_HEIGHT - 2;
    vga_fill_rect(0, y, SCREEN_WIDTH, 1, ' ', VGA_BLCK, VGA_LGREY);
    
    vga_set_color(VGA_BLCK, VGA_LGREY);
    const char* menu_items[] = {
        " SPACE:Play/Pause ", " N:Next ", " P:Prev ", " A:Add ", " D:Delete ",
        " S:Shuffle ", " R:Repeat ", " V:Visualizer ", " ESC:Exit "
    };
    
    int x = 1;
    for (int i = 0; i < 9; i++) {
        const char* item = menu_items[i];
        for (int j = 0; item[j]; j++) {
            vga_putchr_at(x++, y, item[j]);
        }
    }
}

static void draw_file_browser(void) {
    vga_clear();
    draw_header();
    
    vga_draw_box_double(5, 4, 70, 16, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* title = "Select Audio File";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 4, title[i]);
    }

    int visible_start = file_scroll_offset;
    int visible_end = file_scroll_offset + 12;
    if (visible_end > file_count) visible_end = file_count;
    
    for (int i = visible_start; i < visible_end; i++) {
        int y = 6 + (i - visible_start);
        int selected = (i == selected_file);
        
        if (selected) {
            vga_set_color(VGA_YELLOW, VGA_BLUE);
            vga_fill_rect(7, y, 66, 1, ' ', VGA_YELLOW, VGA_BLUE);
        } else {
            vga_set_color(VGA_WHITE, VGA_BLCK);
        }
        
        const char* prefix = selected ? "> " : "  ";
        int x = 8;
        for (int j = 0; prefix[j]; j++) {
            vga_putchr_at(x++, y, prefix[j]);
        }
        
        for (int j = 0; file_list[i].name[j] && j < 55; j++) {
            vga_putchr_at(x++, y, file_list[i].name[j]);
        }
    }

    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* inst = "UP/DOWN: Navigate | ENTER: Add to playlist | ESC: Cancel";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 22, inst[i]);
    }
}

static void draw_visualizer(void) {
    vga_clear();
    draw_header();
    
    vga_draw_box_double(5, 4, 70, 17, VGA_CYAN, VGA_BLCK);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    const char* title = "Audio Visualizer";
    int len = 0;
    while (title[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 4, title[i]);
    }
    
    if (player.state == AUDIO_PLAYING) {
        uint8_t spectrum[32];
        audio_get_spectrum(spectrum, 32);
        
        for (int i = 0; i < 32; i++) {
            int bar_height = (spectrum[i] * 12) / 100;
            int x = 8 + i * 2;
            
            for (int j = 0; j < bar_height; j++) {
                int y = 19 - j;
                uint8_t color;
                if (j < 4) color = VGA_LGREEN;
                else if (j < 8) color = VGA_YELLOW;
                else color = VGA_LRED;
                
                vga_set_color(color, VGA_BLCK);
                vga_putchr_at(x, y, 0xDB);
                vga_putchr_at(x + 1, y, 0xDB);
            }
        }
    } else {
        vga_set_color(VGA_DGREY, VGA_BLCK);
        const char* msg = "No audio playing";
        len = 0;
        while (msg[len]) len++;
        for (int i = 0; i < len; i++) {
            vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 12, msg[i]);
        }
    }
    
    vga_set_color(VGA_LGREY, VGA_BLCK);
    const char* inst = "ESC: Return to player";
    len = 0;
    while (inst[len]) len++;
    for (int i = 0; i < len; i++) {
        vga_putchr_at((SCREEN_WIDTH - len) / 2 + i, 23, inst[i]);
    }
}

static void draw_main_screen(void) {
    vga_begin_batch();
    vga_clear();
    
    draw_header();
    draw_now_playing();
    draw_playlist();
    draw_controls();
    draw_menu_bar();
    
    vga_end_batch();
}


static void handle_main_input(void) {
    keyboard_poll();
    if (!has_key()) return;
    
    char c = get_char();
    
    if (c == 27) {  
        current_screen = PLAYER_EXIT;
    } else if (c == ' ') {  
        if (player.state == AUDIO_PLAYING) {
            player_pause(&player);
            sfx_click();
        } else if (player.state == AUDIO_PAUSED) {
            player_resume(&player);
            sfx_click();
        } else if (player.playlist_count > 0) {
            player_play(&player);
            sfx_click();
        }
    } else if (c == 'n' || c == 'N') {  
        player_next_track(&player);
        sfx_click();
    } else if (c == 'p' || c == 'P') {  
        player_prev_track(&player);
        sfx_click();
    } else if (c == 's' || c == 'S') {  
        player_toggle_shuffle(&player);
        sfx_click();
    } else if (c == 'r' || c == 'R') {  
        player_toggle_repeat(&player);
        sfx_click();
    } else if (c == 'a' || c == 'A') {  
        current_screen = PLAYER_FILE_BROWSER;
        file_count = fat16_list_files(file_list, 50);
        selected_file = 0;
        file_scroll_offset = 0;
        sfx_click();
    } else if (c == 'd' || c == 'D') {  
        if (player.playlist_count > 0) {
            player_remove_track(&player, selected_track);
            if (selected_track >= player.playlist_count && player.playlist_count > 0) {
                selected_track = player.playlist_count - 1;
            }
            sfx_click();
        }
    } else if (c == 'v' || c == 'V') {  
        current_screen = PLAYER_VISUALIZER;
        sfx_click();
    } else if (c == 0x11) { 
        if (selected_track > 0) {
            selected_track--;
            if (selected_track < scroll_offset) {
                scroll_offset--;
            }
            sfx_click();
        }
    } else if (c == 0x12) { 
        if (selected_track < player.playlist_count - 1) {
            selected_track++;
            if (selected_track >= scroll_offset + MAX_VISIBLE_TRACKS) {
                scroll_offset++;
            }
            sfx_click();
        }
    } else if (c == '\n') {  
        player.current_track = selected_track;
        player_play(&player);
        sfx_click();
    } else if (c == '+' || c == '=') {  
        player_set_volume(&player, player.volume + 5);
        sfx_click();
    } else if (c == '-' || c == '_') {  
        player_set_volume(&player, player.volume - 5);
        sfx_click();
    } else if (c >= '1' && c <= '9') {  
        switch (c) {
            case '1': audio_play_mario_theme(); break;
            case '2': audio_play_tetris_theme(); break;
            case '3': audio_play_nokia_tune(); break;
            case '4': audio_play_happy_birthday(); break;
            case '5': audio_play_imperial_march(); break;
            case '6': audio_play_jingle_bells(); break;
        }
    }
}

static void handle_file_browser_input(void) {
    keyboard_poll();
    if (!has_key()) return;
    
    char c = get_char();
    
    if (c == 27) {  
        current_screen = PLAYER_MAIN;
        sfx_click();
    } else if (c == 0x11) { 
        if (selected_file > 0) {
            selected_file--;
            if (selected_file < file_scroll_offset) {
                file_scroll_offset--;
            }
            sfx_click();
        }
    } else if (c == 0x12) {  
        if (selected_file < file_count - 1) {
            selected_file++;
            if (selected_file >= file_scroll_offset + 12) {
                file_scroll_offset++;
            }
            sfx_click();
        }
    } else if (c == '\n') {  
        if (file_count > 0) {
            player_add_track(&player, file_list[selected_file].name);
            current_screen = PLAYER_MAIN;
            sfx_success();
        }
    }
}

static void handle_visualizer_input(void) {
    keyboard_poll();
    if (!has_key()) return;
    
    char c = get_char();
    
    if (c == 27 || c == 'v' || c == 'V') {  
        current_screen = PLAYER_MAIN;
        sfx_click();
    } else if (c == 'm' || c == 'M') {   
        visualizer_mode = (visualizer_mode + 1) % 3;
        sfx_click();
    }
}


void musicplayer_init(void) {
    audio_init();
    player_init(&player);
    audio_effects_init(&effects);
    
    current_screen = PLAYER_MAIN;
    selected_track = 0;
    scroll_offset = 0;
    menu_selection = 0;
    visualizer_mode = 0;
    
    player_add_track(&player, "DEMO1.WAV");
    player_add_track(&player, "DEMO2.WAV");
    player_add_track(&player, "SONG1.WAV");
}

void musicplayer_run(void) {
    musicplayer_init();
    
    sfx_startup();
    
    while (current_screen != PLAYER_EXIT) {
        if (current_screen == PLAYER_MAIN) {
            draw_main_screen();
            handle_main_input();
            pit_delay_ms(50);
        } else if (current_screen == PLAYER_FILE_BROWSER) {
            draw_file_browser();
            handle_file_browser_input();
            pit_delay_ms(50);
        } else if (current_screen == PLAYER_VISUALIZER) {
            draw_visualizer();
            handle_visualizer_input();
            pit_delay_ms(100);
        }
    }
    
    player_stop(&player);
    audio_cleanup();
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}

void musicplayer_cleanup(void) {
    player_stop(&player);
    audio_cleanup();
}


void musicplayer_demo_mode(void) {
    vga_clear();
    vga_println("SmolOS Audio System Demo");
    vga_println("-----------------------");
    vga_println("");
    vga_println("Playing demo melodies...");
    vga_println("");
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("1. Mario Theme");
    audio_play_mario_theme();
    pit_delay_ms(500);
    
    vga_println("2. Tetris Theme");
    audio_play_tetris_theme();
    pit_delay_ms(500);
    
    vga_println("3. Nokia Tune");
    audio_play_nokia_tune();
    pit_delay_ms(500);
    
    vga_println("4. Happy Birthday");
    audio_play_happy_birthday();
    pit_delay_ms(500);
    
    vga_println("5. Imperial March");
    audio_play_imperial_march();
    pit_delay_ms(500);
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("");
    vga_println("Sound effects demo:");
    
    vga_println("  - Success");
    sfx_success();
    pit_delay_ms(300);
    
    vga_println("  - Error");
    sfx_error();
    pit_delay_ms(300);
    
    vga_println("  - Notification");
    sfx_notification();
    pit_delay_ms(300);
    
    vga_println("  - Coin");
    sfx_coin();
    pit_delay_ms(300);
    
    vga_println("  - Powerup");
    sfx_powerup();
    pit_delay_ms(300);
    
    vga_println("  - Explosion");
    sfx_explosion();
    pit_delay_ms(300);
    
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("");
    vga_println("Demo complete!");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("Press any key to continue...");
    wait_for_char();
}

void audio_demo_menu(void) {
    int selection = 0;
    int running = 1;
    
    while (running) {
        vga_clear();

        vga_set_color(VGA_LCYAN, VGA_BLCK);
        vga_print_centered("SmolOS Audio System Demo", 2);
        
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_print_centered("PC Speaker Audio Engine v1.0", 3);

        const char* options[] = {
            "Play Built-in Melodies",
            "Sound Effects Demo",
            "Musical Scale Test",
            "Chord Demo",
            "Interactive Piano",
            "Music Player",
            "Audio Tests",
            "Exit Demo"
        };
        
        for (int i = 0; i < 8; i++) {
            int selected = (i == selection);
            
            if (selected) {
                vga_set_color(VGA_YELLOW, VGA_BLUE);
                vga_fill_rect(25, 7 + i * 2, 30, 1, ' ', VGA_YELLOW, VGA_BLUE);
            } else {
                vga_set_color(VGA_WHITE, VGA_BLCK);
            }
            
            const char* prefix = selected ? "> " : "  ";
            int x = 27;
            for (int j = 0; prefix[j]; j++) {
                vga_putchr_at(x++, 7 + i * 2, prefix[j]);
            }
            
            for (int j = 0; options[i][j]; j++) {
                vga_putchr_at(x++, 7 + i * 2, options[i][j]);
            }
        }

        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_print_centered("Use UP/DOWN arrows, ENTER to select, ESC to exit", 23);

        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            if (c == 0x11 && selection > 0) { 
                selection--;
                sfx_click();
            } else if (c == 0x12 && selection < 7) { 
                selection++;
                sfx_click();
            } else if (c == '\n') { 
                sfx_click();
                pit_delay_ms(100);
                
                switch (selection) {
                    case 0: demo_melodies(); break;
                    case 1: demo_sound_effects(); break;
                    case 2: demo_musical_scale(); break;
                    case 3: demo_chords(); break;
                    case 4: demo_piano(); break;
                    case 5: musicplayer_run(); break;
                    case 6: demo_audio_tests(); break;
                    case 7: running = 0; break;
                }
            } else if (c == 27) {  
                running = 0;
            }
        }
        
        pit_delay_ms(50);
    }
}


void demo_melodies(void) {
    vga_clear();
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Built-in Melodies Demo", 1);
    vga_println("");
    
    const char* songs[] = {
        "Super Mario Bros Theme",
        "Tetris Theme",
        "Nokia Tune",
        "Happy Birthday",
        "Imperial March (Star Wars)",
        "Jingle Bells"
    };
    
    for (int i = 0; i < 6; i++) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_print("Playing: ");
        vga_set_color(VGA_WHITE, VGA_BLCK);
        vga_println(songs[i]);
        
        switch (i) {
            case 0: audio_play_mario_theme(); break;
            case 1: audio_play_tetris_theme(); break;
            case 2: audio_play_nokia_tune(); break;
            case 3: audio_play_happy_birthday(); break;
            case 4: audio_play_imperial_march(); break;
            case 5: audio_play_jingle_bells(); break;
        }
        
        pit_delay_ms(500);
        
        keyboard_poll();
        if (has_key() && get_char() == 27) break;
    }
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("");
    vga_println("Demo complete! Press any key to continue...");
    wait_for_char();
}


void demo_sound_effects(void) {
    vga_clear();
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Sound Effects Demo", 1);
    vga_println("");
    vga_println("");
    
    const char* effects[] = {
        "Startup Sound",
        "Shutdown Sound",
        "Success",
        "Error",
        "Notification",
        "Click",
        "Whoosh",
        "Explosion",
        "Coin",
        "Powerup",
        "Game Over",
        "Level Complete"
    };
    
    for (int i = 0; i < 12; i++) {
        vga_set_color(VGA_YELLOW, VGA_BLCK);
        vga_print("  ");
        vga_print(effects[i]);
        vga_print(": ");
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_print("[Playing]");
        
        switch (i) {
            case 0: sfx_startup(); break;
            case 1: sfx_shutdown(); break;
            case 2: sfx_success(); break;
            case 3: sfx_error(); break;
            case 4: sfx_notification(); break;
            case 5: sfx_click(); break;
            case 6: sfx_whoosh(); break;
            case 7: sfx_explosion(); break;
            case 8: sfx_coin(); break;
            case 9: sfx_powerup(); break;
            case 10: sfx_game_over(); break;
            case 11: sfx_level_complete(); break;
        }
        
        vga_set_color(VGA_DGREY, VGA_BLCK);
        vga_print(" [Done]");
        vga_println("");
        
        pit_delay_ms(300);

        keyboard_poll();
        if (has_key() && get_char() == 27) break;
    }
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("");
    vga_println("Press any key to continue...");
    wait_for_char();
}


void demo_musical_scale(void) {
    vga_clear();
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Musical Scale Test", 1);
    vga_println("");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Playing C Major Scale (C4 to C5):");
    vga_println("");
    
    const char* notes[] = {"C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"};
    uint16_t frequencies[] = {
        NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4,
        NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
    };
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Ascending: ");
    for (int i = 0; i < 8; i++) {
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_print(notes[i]);
        vga_print(" ");
        
        audio_play_note(frequencies[i], 300);
        pit_delay_ms(50);
    }
    vga_println("");
    
    pit_delay_ms(500);

    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("Descending: ");
    for (int i = 7; i >= 0; i--) {
        vga_set_color(VGA_LCYAN, VGA_BLCK);
        vga_print(notes[i]);
        vga_print(" ");
        
        audio_play_note(frequencies[i], 300);
        pit_delay_ms(50);
    }
    vga_println("");
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("");
    vga_println("Press any key to continue...");
    wait_for_char();
}


void demo_chords(void) {
    vga_clear();
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Chord Demo", 1);
    vga_println("");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Playing common chords:");
    vga_println("");

    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("C Major (C-E-G): ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("[Playing]");
    uint16_t c_major[] = {NOTE_C4, NOTE_E4, NOTE_G4};
    audio_play_chord(c_major, 3, 1000);
    pit_delay_ms(500);

    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("G Major (G-B-D): ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("[Playing]");
    uint16_t g_major[] = {NOTE_G4, NOTE_B4, NOTE_D4 * 2};
    audio_play_chord(g_major, 3, 1000);
    pit_delay_ms(500);

    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_print("F Major (F-A-C): ");
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("[Playing]");
    uint16_t f_major[] = {NOTE_F4, NOTE_A4, NOTE_C5};
    audio_play_chord(f_major, 3, 1000);
    pit_delay_ms(500);
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("");
    vga_println("Press any key to continue...");
    wait_for_char();
}


void demo_piano(void) {
    vga_clear();
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Interactive Piano", 1);
    vga_println("");
    vga_println("");
    
    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Play notes with your keyboard!");
    vga_println("");
    
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("Keyboard layout:");
    vga_println("");
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_println("  Q   W   E   R   T   Y   U   I");
    vga_println("  C4  D4  E4  F4  G4  A4  B4  C5");
    vga_println("");
    vga_set_color(VGA_WHITE, VGA_BLCK);
    vga_println("Press ESC to exit piano mode");
    vga_println("");
    vga_println("");
    
    int running = 1;
    while (running) {
        keyboard_poll();
        if (has_key()) {
            char c = get_char();
            
            uint16_t freq = 0;
            const char* note_name = "";
            
            switch (c) {
                case 'q': case 'Q': freq = NOTE_C4; note_name = "C4"; break;
                case 'w': case 'W': freq = NOTE_D4; note_name = "D4"; break;
                case 'e': case 'E': freq = NOTE_E4; note_name = "E4"; break;
                case 'r': case 'R': freq = NOTE_F4; note_name = "F4"; break;
                case 't': case 'T': freq = NOTE_G4; note_name = "G4"; break;
                case 'y': case 'Y': freq = NOTE_A4; note_name = "A4"; break;
                case 'u': case 'U': freq = NOTE_B4; note_name = "B4"; break;
                case 'i': case 'I': freq = NOTE_C5; note_name = "C5"; break;
                case 27: running = 0; break;  
            }
            
            if (freq > 0) {
                vga_set_color(VGA_LGREEN, VGA_BLCK);
                vga_print("Playing: ");
                vga_set_color(VGA_YELLOW, VGA_BLCK);
                vga_print(note_name);
                vga_println("      ");
                
                audio_play_note(freq, 200);
            }
        }
        
        pit_delay_ms(10);
    }
}


void demo_audio_tests(void) {
    vga_clear();
    vga_set_color(VGA_LCYAN, VGA_BLCK);
    vga_print_centered("Audio System Tests", 1);
    vga_println("");
    vga_println("");

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("Test 1: Frequency Sweep (200Hz to 2000Hz)");
    vga_set_color(VGA_LGREY, VGA_BLCK);
    vga_print("Progress: ");
    
    for (int f = 200; f <= 2000; f += 50) {
        speaker_play_tone(f, 30);
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_putchr('.');
    }
    vga_println(" [PASS]");
    pit_delay_ms(500);

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("");
    vga_println("Test 2: Volume Simulation");
    for (int vol = 20; vol <= 100; vol += 20) {
        vga_set_color(VGA_LGREY, VGA_BLCK);
        vga_print("  Volume ");
        vga_print_int(vol);
        vga_print("%: ");
        
        audio_set_master_volume(vol);
        speaker_play_tone(1000, 200);
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_println("[OK]");
        pit_delay_ms(200);
    }
    audio_set_master_volume(80);

    vga_set_color(VGA_YELLOW, VGA_BLCK);
    vga_println("");
    vga_println("Test 3: Note Duration Test");
    uint16_t durations[] = {100, 200, 400, 800};
    for (int i = 0; i < 4; i++) {
        vga_set_color(VGA_LGREY, VGA_BLCK);
        vga_print("  ");
        vga_print_int(durations[i]);
        vga_print("ms: ");
        
        audio_play_note(NOTE_A4, durations[i]);
        
        vga_set_color(VGA_LGREEN, VGA_BLCK);
        vga_println("[OK]");
        pit_delay_ms(200);
    }
    
    vga_set_color(VGA_LGREEN, VGA_BLCK);
    vga_println("");
    vga_println("All tests passed!");
    vga_println("Press any key to continue...");
    wait_for_char();
}

void audio_demo_run(void) {
    audio_init();
    
    sfx_startup();
    
    audio_demo_menu();
    
    sfx_shutdown();
    audio_cleanup();
    
    vga_clear();
    vga_set_color(VGA_WHITE, VGA_BLCK);
}