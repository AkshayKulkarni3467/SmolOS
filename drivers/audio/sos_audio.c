#include "sos_audio.h"
#include "sos_io.h"
#include "sos_pit.h"
#include "sos_fat16.h"
#include "sos_memory.h"
#include "sos_string.h"

static int audio_initialized = 0;
static AudioState current_state = AUDIO_STOPPED;
static int master_volume = 80;
static uint8_t audio_buffer[AUDIO_BUFFER_SIZE];
static uint32_t buffer_position = 0;
static uint32_t buffer_size = 0;


void audio_init(void) {
    if (audio_initialized) return;
    
    speaker_disable();
    
    audio_initialized = 1;
    current_state = AUDIO_STOPPED;
    master_volume = 80;
}

void audio_cleanup(void) {
    speaker_disable();
    current_state = AUDIO_STOPPED;
}


void speaker_enable(void) {
    uint8_t tmp = inb(PC_SPEAKER_PORT);
    if (tmp != (tmp | 3)) {
        outb(PC_SPEAKER_PORT, tmp | 3);
    }
}

void speaker_disable(void) {
    uint8_t tmp = inb(PC_SPEAKER_PORT) & 0xFC;
    outb(PC_SPEAKER_PORT, tmp);
}

uint32_t audio_freq_to_pit_divisor(uint32_t frequency) {
    if (frequency == 0) return 0;
    return 1193180 / frequency;
}

void speaker_set_frequency(uint32_t frequency) {
    if (frequency == 0) {
        speaker_disable();
        return;
    }
    
    uint32_t divisor = audio_freq_to_pit_divisor(frequency);

    outb(PIT_COMMAND, 0xB6);

    outb(PIT_CHANNEL_2, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL_2, (uint8_t)((divisor >> 8) & 0xFF));
    
    speaker_enable();
}

void speaker_play_tone(uint32_t frequency, uint32_t duration_ms) {
    speaker_set_frequency(frequency);
    pit_delay_ms(duration_ms);
    speaker_disable();
}

void speaker_beep(void) {
    speaker_play_tone(800, 100);
}


void audio_play_note(uint16_t frequency, uint16_t duration_ms) {
    if (frequency == 0) {
        pit_delay_ms(duration_ms);
    } else {
        speaker_play_tone(frequency, duration_ms);
    }
}

void audio_play_melody(const Note* melody, int note_count) {
    current_state = AUDIO_PLAYING;
    
    for (int i = 0; i < note_count && current_state == AUDIO_PLAYING; i++) {
        audio_play_note(melody[i].frequency, melody[i].duration);
        pit_delay_us(100); 
    }
    
    current_state = AUDIO_STOPPED;
}

void audio_play_chord(const uint16_t* frequencies, int freq_count, uint16_t duration_ms) {
    uint32_t start_time = pit_get_total_milliseconds();
    int freq_index = 0;
    
    while (pit_get_total_milliseconds() - start_time < duration_ms) {
        speaker_set_frequency(frequencies[freq_index]);
        pit_delay_ms(5);
        freq_index = (freq_index + 1) % freq_count;
    }
    
    speaker_disable();
}


int audio_load_wav(const char* filename) {
    uint32_t file_size;
    char* file_data = fat16_read_file(filename, &file_size);
    
    if (!file_data || file_size < sizeof(WAV_Header) + sizeof(WAV_Format) + sizeof(WAV_Data)) {
        return -1;
    }
    
    WAV_Header* header = (WAV_Header*)file_data;

    if (header->riff[0] != 'R' || header->riff[1] != 'I' || 
        header->riff[2] != 'F' || header->riff[3] != 'F' ||
        header->wave[0] != 'W' || header->wave[1] != 'A' ||
        header->wave[2] != 'V' || header->wave[3] != 'E') {
        return -2;
    }

    buffer_size = (file_size < AUDIO_BUFFER_SIZE) ? file_size : AUDIO_BUFFER_SIZE;
    memcpy(audio_buffer, file_data, buffer_size);
    buffer_position = 0;
    
    return 0;
}

int audio_play_wav(const char* filename) {
    if (audio_load_wav(filename) != 0) {
        return -1;
    }

    current_state = AUDIO_PLAYING;

    speaker_play_tone(1000, 500);
    
    current_state = AUDIO_STOPPED;
    return 0;
}

int audio_get_wav_info(const char* filename, AudioFileInfo* info) {
    uint32_t file_size;
    char* file_data = fat16_read_file(filename, &file_size);
    
    if (!file_data) return -1;
    
    WAV_Header* header = (WAV_Header*)file_data;
    WAV_Format* format = (WAV_Format*)(file_data + sizeof(WAV_Header));
    WAV_Data* data = (WAV_Data*)(file_data + sizeof(WAV_Header) + sizeof(WAV_Format));
    
    strcpy(info->filename, filename);
    info->sample_rate = format->sample_rate;
    info->channels = format->num_channels;
    info->bits_per_sample = format->bits_per_sample;
    info->file_size = file_size;
    info->duration_seconds = data->data_size / format->byte_rate;
    
    return 0;
}

void audio_stop_playback(void) {
    current_state = AUDIO_STOPPED;
    speaker_disable();
}

void audio_pause_playback(void) {
    if (current_state == AUDIO_PLAYING) {
        current_state = AUDIO_PAUSED;
        speaker_disable();
    }
}

void audio_resume_playback(void) {
    if (current_state == AUDIO_PAUSED) {
        current_state = AUDIO_PLAYING;
    }
}


void player_init(AudioPlayer* player) {
    player->state = AUDIO_STOPPED;
    player->current_track = 0;
    player->playlist_count = 0;
    player->shuffle = 0;
    player->repeat = 0;
    player->volume = 80;
    player->playback_position = 0;
    player->total_duration = 0;
    
    for (int i = 0; i < MAX_PLAYLIST_SIZE; i++) {
        player->playlist[i].filename[0] = '\0';
        player->playlist[i].duration = 0;
        player->playlist[i].is_loaded = 0;
    }
}

int player_add_track(AudioPlayer* player, const char* filename) {
    if (player->playlist_count >= MAX_PLAYLIST_SIZE) {
        return -1;
    }
    
    int index = player->playlist_count;
    strcpy(player->playlist[index].filename, filename);
    player->playlist[index].duration = 0;
    player->playlist[index].is_loaded = 1;
    player->playlist_count++;
    
    return index;
}

int player_remove_track(AudioPlayer* player, int index) {
    if (index < 0 || index >= player->playlist_count) {
        return -1;
    }

    for (int i = index; i < player->playlist_count - 1; i++) {
        player->playlist[i] = player->playlist[i + 1];
    }
    
    player->playlist_count--;
    
    if (player->current_track >= player->playlist_count && player->playlist_count > 0) {
        player->current_track = player->playlist_count - 1;
    }
    
    return 0;
}

void player_clear_playlist(AudioPlayer* player) {
    player->playlist_count = 0;
    player->current_track = 0;
    player_stop(player);
}

void player_play(AudioPlayer* player) {
    if (player->playlist_count == 0) return;
    
    player->state = AUDIO_PLAYING;

    const char* filename = player->playlist[player->current_track].filename;
    audio_play_wav(filename);
}

void player_pause(AudioPlayer* player) {
    if (player->state == AUDIO_PLAYING) {
        player->state = AUDIO_PAUSED;
        speaker_disable();
    }
}

void player_resume(AudioPlayer* player) {
    if (player->state == AUDIO_PAUSED) {
        player->state = AUDIO_PLAYING;
        speaker_enable();
    }
}

void player_stop(AudioPlayer* player) {
    player->state = AUDIO_STOPPED;
    player->playback_position = 0;
    speaker_disable();
}

void player_next_track(AudioPlayer* player) {
    if (player->playlist_count == 0) return;
    
    player->current_track++;
    if (player->current_track >= player->playlist_count) {
        if (player->repeat) {
            player->current_track = 0;
        } else {
            player->current_track = player->playlist_count - 1;
            player_stop(player);
            return;
        }
    }
    
    if (player->state == AUDIO_PLAYING) {
        player_play(player);
    }
}

void player_prev_track(AudioPlayer* player) {
    if (player->playlist_count == 0) return;
    
    player->current_track--;
    if (player->current_track < 0) {
        player->current_track = player->repeat ? player->playlist_count - 1 : 0;
    }
    
    if (player->state == AUDIO_PLAYING) {
        player_play(player);
    }
}

void player_set_volume(AudioPlayer* player, int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    player->volume = volume;
    master_volume = volume;
}

void player_toggle_shuffle(AudioPlayer* player) {
    player->shuffle = !player->shuffle;
}

void player_toggle_repeat(AudioPlayer* player) {
    player->repeat = !player->repeat;
}

int player_get_progress(AudioPlayer* player) {
    if (player->total_duration == 0) return 0;
    return (player->playback_position * 100) / player->total_duration;
}

void player_seek(AudioPlayer* player, int position_percent) {
    if (position_percent < 0) position_percent = 0;
    if (position_percent > 100) position_percent = 100;
    player->playback_position = (player->total_duration * position_percent) / 100;
}


void sfx_startup(void) {
    Note startup[] = {
        {NOTE_C4, 100},
        {NOTE_E4, 100},
        {NOTE_G4, 100},
        {NOTE_C5, 200}
    };
    audio_play_melody(startup, 4);
}

void sfx_shutdown(void) {
    Note shutdown[] = {
        {NOTE_C5, 100},
        {NOTE_G4, 100},
        {NOTE_E4, 100},
        {NOTE_C4, 200}
    };
    audio_play_melody(shutdown, 4);
}

void sfx_error(void) {
    Note error[] = {
        {200, 100},
        {150, 100},
        {100, 200}
    };
    audio_play_melody(error, 3);
}

void sfx_success(void) {
    Note success[] = {
        {NOTE_C4, 80},
        {NOTE_E4, 80},
        {NOTE_G4, 80},
        {NOTE_C5, 160}
    };
    audio_play_melody(success, 4);
}

void sfx_notification(void) {
    speaker_play_tone(1000, 50);
    pit_delay_ms(20);
    speaker_play_tone(1200, 50);
}

void sfx_click(void) {
    speaker_play_tone(800, 20);
}

void sfx_whoosh(void) {
    for (int f = 2000; f > 200; f -= 100) {
        speaker_play_tone(f, 10);
    }
}

void sfx_explosion(void) {
    for (int f = 800; f > 100; f -= 50) {
        speaker_play_tone(f, 15);
    }
}

void sfx_coin(void) {
    Note coin[] = {
        {NOTE_B4, 50},
        {NOTE_E4 * 2, 100}
    };
    audio_play_melody(coin, 2);
}

void sfx_powerup(void) {
    Note powerup[] = {
        {NOTE_G4, 60},
        {NOTE_B4, 60},
        {NOTE_E4 * 2, 60},
        {NOTE_G4 * 2, 120}
    };
    audio_play_melody(powerup, 4);
}

void sfx_game_over(void) {
    Note game_over[] = {
        {NOTE_C4, 200},
        {NOTE_G4 - 50, 200},
        {NOTE_E4, 200},
        {NOTE_A4 - 100, 200},
        {NOTE_B4 - 100, 200},
        {NOTE_A4 - 100, 200},
        {NOTE_GS0 * OCTAVE_4, 300},
        {NOTE_AS0 * OCTAVE_4, 300},
        {NOTE_GS0 * OCTAVE_4, 600}
    };
    audio_play_melody(game_over, 9);
}

void sfx_level_complete(void) {
    Note complete[] = {
        {NOTE_G4, 100},
        {NOTE_C5, 100},
        {NOTE_E4 * 2, 100},
        {NOTE_G4 * 2, 100},
        {NOTE_C5 * 2, 400}
    };
    audio_play_melody(complete, 5);
}


void audio_play_startup_tune(void) {
    sfx_startup();
}

void audio_play_shutdown_tune(void) {
    sfx_shutdown();
}

void audio_play_mario_theme(void) {
    Note mario[] = {
        {NOTE_E4 * 2, 125}, {NOTE_E4 * 2, 125}, {0, 125}, {NOTE_E4 * 2, 125},
        {0, 125}, {NOTE_C4 * 2, 125}, {NOTE_E4 * 2, 125}, {0, 125},
        {NOTE_G4 * 2, 125}, {0, 375}, {NOTE_G4, 125}, {0, 375}
    };
    audio_play_melody(mario, 12);
}

void audio_play_tetris_theme(void) {
    Note tetris[] = {
        {NOTE_E4 * 2, 200}, {NOTE_B4, 100}, {NOTE_C4 * 2, 100}, {NOTE_D4 * 2, 200},
        {NOTE_C4 * 2, 100}, {NOTE_B4, 100}, {NOTE_A4, 200}, {NOTE_A4, 100},
        {NOTE_C4 * 2, 100}, {NOTE_E4 * 2, 200}, {NOTE_D4 * 2, 100}, {NOTE_C4 * 2, 100},
        {NOTE_B4, 300}, {NOTE_C4 * 2, 100}, {NOTE_D4 * 2, 200}, {NOTE_E4 * 2, 200}
    };
    audio_play_melody(tetris, 16);
}

void audio_play_nokia_tune(void) {
    Note nokia[] = {
        {NOTE_E4 * 2, 125}, {NOTE_D4 * 2, 125}, {NOTE_FS0 * OCTAVE_4, 250}, {NOTE_GS0 * OCTAVE_4, 250},
        {NOTE_CS0 * OCTAVE_5, 125}, {NOTE_B4, 125}, {NOTE_D4, 250}, {NOTE_E4, 250},
        {NOTE_B4, 125}, {NOTE_A4, 125}, {NOTE_CS0 * OCTAVE_4, 250}, {NOTE_E4, 250},
        {NOTE_A4, 500}
    };
    audio_play_melody(nokia, 13);
}

void audio_play_happy_birthday(void) {
    Note birthday[] = {
        {NOTE_C4, 250}, {NOTE_C4, 250}, {NOTE_D4, 500}, {NOTE_C4, 500},
        {NOTE_F4, 500}, {NOTE_E4, 1000}, {NOTE_C4, 250}, {NOTE_C4, 250},
        {NOTE_D4, 500}, {NOTE_C4, 500}, {NOTE_G4, 500}, {NOTE_F4, 1000}
    };
    audio_play_melody(birthday, 12);
}

void audio_play_imperial_march(void) {
    Note imperial[] = {
        {NOTE_A4, 500}, {NOTE_A4, 500}, {NOTE_A4, 500}, {NOTE_F4, 350},
        {NOTE_C5, 150}, {NOTE_A4, 500}, {NOTE_F4, 350}, {NOTE_C5, 150},
        {NOTE_A4, 1000}, {NOTE_E4 * 2, 500}, {NOTE_E4 * 2, 500}, {NOTE_E4 * 2, 500},
        {NOTE_F4 * 2, 350}, {NOTE_C5, 150}, {NOTE_GS0 * OCTAVE_4, 500}, {NOTE_F4, 350}
    };
    audio_play_melody(imperial, 16);
}

void audio_play_jingle_bells(void) {
    Note jingle[] = {
        {NOTE_E4, 250}, {NOTE_E4, 250}, {NOTE_E4, 500}, {NOTE_E4, 250},
        {NOTE_E4, 250}, {NOTE_E4, 500}, {NOTE_E4, 250}, {NOTE_G4, 250},
        {NOTE_C4, 375}, {NOTE_D4, 125}, {NOTE_E4, 1000}
    };
    audio_play_melody(jingle, 11);
}


int audio_is_playing(void) {
    return current_state == AUDIO_PLAYING;
}

AudioState audio_get_state(void) {
    return current_state;
}

void audio_set_master_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    master_volume = volume;
}

int audio_get_master_volume(void) {
    return master_volume;
}


void audio_effects_init(AudioEffects* effects) {
    effects->eq_enabled = 0;
    for (int i = 0; i < EQ_BAND_COUNT; i++) {
        effects->eq_levels[i] = 0;
    }
    effects->echo_enabled = 0;
    effects->echo_delay = 100;
    effects->reverb_enabled = 0;
}

void audio_apply_effects(AudioEffects* effects, uint8_t* buffer, uint32_t size) {
    if (!effects->eq_enabled && !effects->echo_enabled && !effects->reverb_enabled) {
        return;
    }
    
    if (effects->eq_enabled) {
        for (uint32_t i = 0; i < size; i++) {
            int sample = buffer[i] - 128;
            sample = sample * (100 + effects->eq_levels[EQ_MID]) / 100;
            if (sample > 127) sample = 127;
            if (sample < -128) sample = -128;
            buffer[i] = sample + 128;
        }
    }
}

void audio_set_eq_band(AudioEffects* effects, EQBand band, int level) {
    if (band >= 0 && band < EQ_BAND_COUNT) {
        if (level < -10) level = -10;
        if (level > 10) level = 10;
        effects->eq_levels[band] = level;
    }
}

void audio_toggle_echo(AudioEffects* effects) {
    effects->echo_enabled = !effects->echo_enabled;
}

void audio_toggle_reverb(AudioEffects* effects) {
    effects->reverb_enabled = !effects->reverb_enabled;
}


void audio_get_spectrum(uint8_t* spectrum_data, int bar_count) {
    for (int i = 0; i < bar_count; i++) {
        spectrum_data[i] = (buffer_position + i * 10) % 100;
    }
}

