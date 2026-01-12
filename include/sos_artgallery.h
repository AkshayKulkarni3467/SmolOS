#ifndef INCLUDE_SMOLOS_ARTGALLERY_H
#define INCLUDE_SMOLOS_ARTGALLERY_H

#include "sos_stdint.h"

typedef enum {
    ART_MATRIX_RAIN,
    ART_PLASMA,
    ART_FIRE,
    ART_WAVES,
    ART_STARFIELD,
    ART_MANDELBROT,
    ART_TUNNEL,
    ART_METABALLS,
    ART_PARTICLES,
    ART_DNA_HELIX,
    ART_RADAR,
    ART_SPIRAL,
    ART_COUNT
} ArtType;

typedef struct {
    const char* name;
    const char* description;
    void (*render_func)(void);
} ArtPiece;

void art_gallery_main(void);

void art_matrix_rain(void);
void art_plasma(void);
void art_fire(void);
void art_waves(void);
void art_starfield(void);
void art_mandelbrot(void);
void draw_branch(int x, int y, int len, int angle, int depth, uint8_t color);
void art_tunnel(void);
void art_metaballs(void);
void art_particles(void);
void art_dna_helix(void);
void art_radar(void);
void art_spiral_galaxy(void);

void art_init_random(void);
uint32_t art_rand(void);
uint8_t art_get_color(int value, int max);
void art_fade_screen(void);

#endif // INCLUDE_SMOLOS_ARTGALLERY_H