/*
 * asteroids.h — Interface du module astéroïdes (Phase 4, port Neo6502)
 */

#ifndef ASTEROIDS_H
#define ASTEROIDS_H

#define MAX_ASTEROIDS 6

#define SIZE_SMALL    0
#define SIZE_MEDIUM   1
#define SIZE_LARGE    2

typedef struct {
    int           x, y;
    int           prev_x, prev_y;  /* pos en sortie de frame précédente (erase) */
    unsigned char x_frac, y_frac;  /* 8.8 fixed-point partie basse */
    int           vx, vy;          /* 8.8 signé */
    unsigned char shape;           /* 0-3 */
    unsigned char size;            /* SIZE_SMALL / MEDIUM / LARGE */
    unsigned char active;
    unsigned char drawn;           /* 1 si actuellement tracé à prev_x, prev_y */
} Asteroid;

extern Asteroid asteroids[MAX_ASTEROIDS];
extern unsigned char current_wave;
extern unsigned char scr_speedup;
extern unsigned char ast_break_timer;

/* shapes.c (généré) */
extern const unsigned char shape_nverts[12];
extern const signed char  shape_vx[12][14];
extern const signed char  shape_vy[12][14];
extern const unsigned char shape_radii[3];

void asteroids_init(unsigned char seed);
void asteroids_spawn_wave(void);
void asteroids_update(void);
void asteroids_draw(void);
void asteroids_render(void);
void asteroids_fragment(unsigned char idx);
unsigned char asteroids_count(void);
unsigned char rng8(void);

#endif /* ASTEROIDS_H */
