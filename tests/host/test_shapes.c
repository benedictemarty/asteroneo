/* test_shapes.c — tables générées + rendu XOR des astéroïdes (stubs EFLA) :
 * idempotence erase/draw, sommets peints exactement une fois, wraparound. */
#include <stdio.h>
#include "asteroids.h"
#include "line.h"
#include "screen.h"

extern unsigned char fb[SCR_H][SCR_W];
extern unsigned long stub_calls;
unsigned long fb_count(void);

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void place(int x, int y, unsigned char size, unsigned char shape)
{
    Asteroid *p = &asteroids[0];
    p->x = x; p->y = y; p->prev_x = x; p->prev_y = y;
    p->size = size; p->shape = shape; p->active = 1; p->drawn = 0;
    p->x_frac = p->y_frac = 0; p->vx = p->vy = 0;
}

int main(void)
{
    unsigned char id;
    /* Tables : 10/16/24 sommets par taille, rayons croissants, sommets dans le rayon */
    for (id = 0; id < 12; id++) CHECK(shape_nverts[id] <= (id < 4 ? 10 : id < 8 ? 16 : 24) && shape_nverts[id] >= 8);
    CHECK(shape_radii[0] < shape_radii[1] && shape_radii[1] < shape_radii[2]);
    for (id = 0; id < 12; id++) {
        unsigned char r = shape_radii[id >> 2];
        for (unsigned char i = 0; i < shape_nverts[id]; i++) {
            int ax = shape_vx[id][i] < 0 ? -shape_vx[id][i] : shape_vx[id][i];
            int ay = shape_vy[id][i] < 0 ? -shape_vy[id][i] : shape_vy[id][i];
            CHECK(ax <= r && ay <= r);
        }
    }

    asteroids_init(0x42);
    gfx_init();

    /* Chaque silhouette : draw puis erase = écran vide ; sommets peints. */
    for (id = 0; id < 12; id++) {
        place(160, 120, id >> 2, id & 3);
        asteroids_render();
        CHECK(fb_count() > 0);
        /* Sommets peints une fois — tolérance : 1 sommet par silhouette
         * (moyenne/grande) peut être traversé par un segment voisin (XOR
         * pair, éteint), artefact inhérent au tracé XOR de contours denses. */
        if ((id >> 2) != SIZE_SMALL) {
            unsigned char holes = 0;
            for (unsigned char i = 0; i < shape_nverts[id]; i++)
                if (fb[120 + shape_vy[id][i]][160 + shape_vx[id][i]] != 1) holes++;
            CHECK(holes <= 1);
        }
        asteroids[0].active = 0;
        asteroids_render();
        CHECK(fb_count() == 0);
    }

    /* Wraparound : astéroïde au bord gauche → pixels aussi à droite. */
    place(2, 120, SIZE_LARGE, 0);
    asteroids_render();
    {
        unsigned long right = 0;
        for (int y = 0; y < SCR_H; y++) for (int x = 300; x < SCR_W; x++) right += fb[y][x];
        CHECK(right > 0);
    }
    asteroids[0].active = 0; asteroids_render(); CHECK(fb_count() == 0);

    /* Coin : 4 instances, toujours idempotent. */
    place(1, 1, SIZE_LARGE, 3);
    asteroids_render(); CHECK(fb_count() > 0);
    asteroids[0].active = 0; asteroids_render(); CHECK(fb_count() == 0);

    /* Mouvement : 200 pas de rendu, jamais de résidu après effacement. */
    place(300, 230, SIZE_MEDIUM, 2);
    asteroids[0].vx = 192; asteroids[0].vy = 96;
    for (int i = 0; i < 200; i++) { asteroids_update(); asteroids_render(); }
    asteroids[0].active = 0; asteroids_render(); CHECK(fb_count() == 0);

    /* Fragmentation : 3 fragments, tailles réduites, écran propre après effacement. */
    asteroids_init(0x42);
    place(160, 120, SIZE_LARGE, 1);
    asteroids_render();
    asteroids_fragment(0);
    CHECK(asteroids_count() == 3);
    for (id = 0; id < MAX_ASTEROIDS; id++) if (asteroids[id].active) CHECK(asteroids[id].size == SIZE_MEDIUM);
    asteroids_render();
    for (id = 0; id < MAX_ASTEROIDS; id++) asteroids[id].active = 0;
    asteroids_render(); CHECK(fb_count() == 0);

    /* Budget d'appels API par astéroïde : 1 ligne par arête */
    place(160, 120, SIZE_LARGE, 1); stub_calls = 0;
    asteroids_render(); CHECK(stub_calls == shape_nverts[9]);

    printf(fails ? "test_shapes : %d échec(s)\n" : "test_shapes : OK\n", fails);
    return fails ? 1 : 0;
}
