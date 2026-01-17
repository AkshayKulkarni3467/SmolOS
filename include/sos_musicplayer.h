#ifndef INCLUDE_SMOLOS_MOUSE_H
#define INCLUDE_SMOLOS_MOUSE_H

#include "sos_stdint.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define PLAYLIST_START_Y 7
#define PLAYLIST_HEIGHT 12
#define MAX_VISIBLE_TRACKS 10

typedef enum {
    PLAYER_MENU = 0,
    PLAYER_MAIN,
    PLAYER_PLAYLIST,
    PLAYER_FILE_BROWSER,
    PLAYER_SETTINGS,
    PLAYER_VISUALIZER,
    PLAYER_EXIT
} PlayerScreen;

void musicplayer_init(void);
void musicplayer_run(void);
void musicplayer_cleanup(void);
void musicplayer_demo_mode(void);
void audio_demo_menu(void);
void demo_melodies(void);
void demo_sound_effects(void);
void demo_musical_scale(void);
void demo_chords(void);
void demo_piano(void);
void demo_audio_tests(void);
void audio_demo_run(void);

#endif // INCLUDE_SMOLOS_MOUSE_H