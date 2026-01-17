#ifndef INCLUDE_SMOLOS_AUDIO_H
#define INCLUDE_SMOLOS_AUDIO_H

#include "sos_stdint.h"

#define PIT_CHANNEL_2   0x42
#define PIT_COMMAND     0x43
#define PC_SPEAKER_PORT 0x61

#define SAMPLE_RATE_8KHZ   8000
#define SAMPLE_RATE_11KHZ  11025
#define SAMPLE_RATE_22KHZ  22050
#define SAMPLE_RATE_44KHZ  44100

#define MAX_PLAYLIST_SIZE  50
#define MAX_FILENAME_LEN   64
#define AUDIO_BUFFER_SIZE  4096

#define NOTE_C0   16
#define NOTE_CS0  17
#define NOTE_D0   18
#define NOTE_DS0  19
#define NOTE_E0   21
#define NOTE_F0   22
#define NOTE_FS0  23
#define NOTE_G0   25
#define NOTE_GS0  26
#define NOTE_A0   28
#define NOTE_AS0  29
#define NOTE_B0   31

#define OCTAVE_0  1
#define OCTAVE_1  2
#define OCTAVE_2  4
#define OCTAVE_3  8
#define OCTAVE_4  16
#define OCTAVE_5  32
#define OCTAVE_6  64
#define OCTAVE_7  128

#define NOTE_C4   (NOTE_C0 * OCTAVE_4)   // 261 Hz
#define NOTE_D4   (NOTE_D0 * OCTAVE_4)   // 293 Hz
#define NOTE_E4   (NOTE_E0 * OCTAVE_4)   // 329 Hz
#define NOTE_F4   (NOTE_F0 * OCTAVE_4)   // 349 Hz
#define NOTE_G4   (NOTE_G0 * OCTAVE_4)   // 392 Hz
#define NOTE_A4   (NOTE_A0 * OCTAVE_4)   // 440 Hz
#define NOTE_B4   (NOTE_B0 * OCTAVE_4)   // 493 Hz
#define NOTE_C5   (NOTE_C0 * OCTAVE_5)   // 523 Hz

typedef struct {
    uint16_t frequency;  
    uint16_t duration;   
} Note;

typedef struct __attribute__((packed)) {
    char riff[4];           
    uint32_t chunk_size;
    char wave[4];           
} WAV_Header;

typedef struct __attribute__((packed)) {
    char fmt[4];            
    uint32_t subchunk_size;
    uint16_t audio_format;  
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} WAV_Format;

typedef struct __attribute__((packed)) {
    char data[4];           
    uint32_t data_size;
} WAV_Data;

typedef enum {
    AUDIO_STOPPED = 0,
    AUDIO_PLAYING,
    AUDIO_PAUSED
} AudioState;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    uint32_t duration_seconds;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t bits_per_sample;
    uint32_t file_size;
} AudioFileInfo;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    uint32_t duration;
    int is_loaded;
} PlaylistEntry;

typedef struct {
    AudioState state;
    int current_track;
    int playlist_count;
    PlaylistEntry playlist[MAX_PLAYLIST_SIZE];
    int shuffle;
    int repeat;
    int volume;  
    uint32_t playback_position;
    uint32_t total_duration;
} AudioPlayer;

typedef enum {
    EQ_BASS = 0,
    EQ_MID,
    EQ_TREBLE,
    EQ_BAND_COUNT
} EQBand;

typedef struct {
    int eq_enabled;
    int eq_levels[EQ_BAND_COUNT];  
    int echo_enabled;
    int echo_delay;
    int reverb_enabled;
} AudioEffects;


void audio_init(void);
void audio_cleanup(void);

void speaker_enable(void);
void speaker_disable(void);
void speaker_set_frequency(uint32_t frequency);
void speaker_play_tone(uint32_t frequency, uint32_t duration_ms);
void speaker_beep(void);

void audio_play_note(uint16_t frequency, uint16_t duration_ms);
void audio_play_melody(const Note* melody, int note_count);
void audio_play_chord(const uint16_t* frequencies, int freq_count, uint16_t duration_ms);


int audio_load_wav(const char* filename);
int audio_play_wav(const char* filename);
int audio_get_wav_info(const char* filename, AudioFileInfo* info);
void audio_stop_playback(void);
void audio_pause_playback(void);
void audio_resume_playback(void);


void player_init(AudioPlayer* player);
int player_add_track(AudioPlayer* player, const char* filename);
int player_remove_track(AudioPlayer* player, int index);
void player_clear_playlist(AudioPlayer* player);
void player_play(AudioPlayer* player);
void player_pause(AudioPlayer* player);
void player_resume(AudioPlayer* player);
void player_stop(AudioPlayer* player);
void player_next_track(AudioPlayer* player);
void player_prev_track(AudioPlayer* player);
void player_set_volume(AudioPlayer* player, int volume);
void player_toggle_shuffle(AudioPlayer* player);
void player_toggle_repeat(AudioPlayer* player);
int player_get_progress(AudioPlayer* player);
void player_seek(AudioPlayer* player, int position_percent);


void sfx_startup(void);
void sfx_shutdown(void);
void sfx_error(void);
void sfx_success(void);
void sfx_notification(void);
void sfx_click(void);
void sfx_whoosh(void);
void sfx_explosion(void);
void sfx_coin(void);
void sfx_powerup(void);
void sfx_game_over(void);
void sfx_level_complete(void);


void audio_play_startup_tune(void);
void audio_play_shutdown_tune(void);
void audio_play_mario_theme(void);
void audio_play_tetris_theme(void);
void audio_play_nokia_tune(void);
void audio_play_happy_birthday(void);
void audio_play_imperial_march(void);
void audio_play_jingle_bells(void);


uint32_t audio_freq_to_pit_divisor(uint32_t frequency);
int audio_is_playing(void);
AudioState audio_get_state(void);
void audio_set_master_volume(int volume);
int audio_get_master_volume(void);


void audio_effects_init(AudioEffects* effects);
void audio_apply_effects(AudioEffects* effects, uint8_t* buffer, uint32_t size);
void audio_set_eq_band(AudioEffects* effects, EQBand band, int level);
void audio_toggle_echo(AudioEffects* effects);
void audio_toggle_reverb(AudioEffects* effects);


void audio_get_spectrum(uint8_t* spectrum_data, int bar_count);


#endif // INCLUDE_SMOLOS_AUDIO_H