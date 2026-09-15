/*
 * asteroids.c — Astéroïdes (port Neo6502 d'Astéroric)
 *
 * Tableau d'astéroïdes en BSS. Chacun = (x, y, vx, vy, shape, size, active).
 * Rendu : polygones vectoriels arcade rev 4 (11-13 sommets, shapes.c)
 * tracés en segments XOR semi-ouverts par l'API graphique — sur Oric les
 * silhouettes étaient blittées en bitmaps pré-rendus (Phase 26), inutile
 * ici : le tracé est fait par le RP2040.
 * Mouvement : intégration 8.8 + wraparound par duplication d'instance.
 * Fragmentation : 1 parent réduit + 2 enfants = 3 fragments par hit.
 */

#include "asteroids.h"
#include "line.h"
#include "phys.h"
#include "screen.h"

Asteroid asteroids[MAX_ASTEROIDS];

/* Phase 10c — wave counter arcade */
unsigned char current_wave;
static unsigned char ast_per_wave;

/* Phase 10h — ScrSpeedup arcade ($02FD) : init 5, +1 par vague, max 11. */
unsigned char scr_speedup;

/* Phase 10k — AstBreakTimer arcade ($02F9) : frames après destruction
 * d'un asteroid pendant lesquelles le saucer ne peut pas spawn. */
unsigned char ast_break_timer;

/* RNG : LFSR 8-bit Galois (polynôme x^8 + x^6 + x^5 + x^4 + 1) */
static unsigned char rng_state;

unsigned char rng8(void)
{
    unsigned char lsb = rng_state & 1;
    rng_state >>= 1;
    if (lsb) rng_state ^= 0xB8;
    return rng_state;
}

void asteroids_init(unsigned char seed)
{
    register Asteroid *p;
    unsigned char i;
    rng_state = seed ? seed : 0x42;
    current_wave = 0;
    ast_per_wave = 0;
    scr_speedup = 5;
    ast_break_timer = 0;
    for (i = 0, p = asteroids; i < MAX_ASTEROIDS; i++, p++) {
        p->active = 0;
        p->drawn  = 0;
    }
}

/* Spawn d'une vague — port de InitWaveVars / InitWaveAsteroids ($7168+) :
 * shape aléatoire, position sur un bord (haut/bas ou gauche/droite selon
 * un bit RNG), vélocité aléatoire. Zones « safe » adaptées à 320 × 240. */
void asteroids_spawn_wave(void)
{
    unsigned char i, n, r;

    if (current_wave == 0) {
        current_wave = 1;
        ast_per_wave = 3;          /* réduit pour cap MAX_ASTEROIDS=6 (fragmentation × 3) */
    } else {
        ast_per_wave++;
        if (ast_per_wave > 5) ast_per_wave = 5;
        current_wave++;
        if (scr_speedup < 11) scr_speedup++;
    }
    n = ast_per_wave;
    if (n > MAX_ASTEROIDS) n = MAX_ASTEROIDS;

    {
    register Asteroid *p = asteroids;
    for (i = 0; i < n; i++, p++) {
        p->active = 1;
        p->drawn  = 0;
        p->size   = SIZE_LARGE;
        p->shape  = (rng8() >> 3) & 3;

        r = rng8();
        if (r & 1) {
            /* Bord gauche/droit, Y aléatoire hors HUD */
            p->x = (r & 2) ? 16 : SCR_W - 20;
            p->y = 30 + (rng8() % 180);
        } else {
            /* Bord haut/bas, X aléatoire */
            p->x = 24 + rng8();             /* 24..279 */
            p->y = (r & 2) ? 24 : SCR_H - 24;
        }
        p->prev_x = p->x;
        p->prev_y = p->y;
        p->x_frac = 0;
        p->y_frac = 0;

        /* Vélocité 8.8 : 96 ou 192 = 0,375 ou 0,75 px/frame (tuning Phase 37). */
        r = rng8();
        p->vx = ((r & 1) ? 1 : -1) * ((r & 2) ? 192 : 96);
        p->vy = ((r & 4) ? 1 : -1) * ((r & 8) ? 192 : 96);
    }
    for (; i < MAX_ASTEROIDS; i++, p++) {
        p->active = 0;
        p->drawn  = 0;
    }
    }
}

void asteroids_update(void)
{
    register Asteroid *p;
    unsigned char i;
    for (i = 0, p = asteroids; i < MAX_ASTEROIDS; i++, p++) {
        if (!p->active) continue;
        p->prev_x = p->x;
        p->prev_y = p->y;
        integrate(&p->x, &p->x_frac, p->vx, SCR_W);
        integrate(&p->y, &p->y_frac, p->vy, SCR_H);
    }
}

/* Polygone fermé de la silhouette id centré en (cx, cy) : chaque sommet
 * est l'arrivée d'exactement un segment → XOR-é une fois. Tracé et clip
 * par segment en asm (poly_xor, neo_gfx.s). */
static void asteroid_poly_at(unsigned char id, int cx, int cy)
{
    poly_n  = shape_nverts[id];
    poly_vx = shape_vx[id];
    poly_vy = shape_vy[id];
    poly_cx = cx;
    poly_cy = cy;
    poly_xor();
}

/* Phase 10l — duplication d'instance : un asteroid proche d'un bord est
 * aussi dessiné à son emplacement fantôme de l'autre côté (coins : 4). */
static void asteroid_draw_one(const Asteroid *p, int cx, int cy)
{
    unsigned char id = (unsigned char)((p->size << 2) + p->shape);
    unsigned char r = shape_radii[p->size];
    int dup_x = 0, dup_y = 0;

    asteroid_poly_at(id, cx, cy);

    if (cx < r)                  dup_x = +SCR_W;
    else if (cx > SCR_XMAX - r)  dup_x = -SCR_W;
    if (cy < r)                  dup_y = +SCR_H;
    else if (cy > SCR_YMAX - r)  dup_y = -SCR_H;

    if (dup_x) asteroid_poly_at(id, cx + dup_x, cy);
    if (dup_y) asteroid_poly_at(id, cx, cy + dup_y);
    if (dup_x && dup_y) asteroid_poly_at(id, cx + dup_x, cy + dup_y);
}

void asteroids_draw(void)
{
    register Asteroid *p;
    unsigned char i;
    for (i = 0, p = asteroids; i < MAX_ASTEROIDS; i++, p++) {
        if (p->active) {
            asteroid_draw_one(p, p->x, p->y);
            p->prev_x = p->x;
            p->prev_y = p->y;
            p->drawn  = !p->drawn;
        }
    }
}

void asteroids_render(void)
{
    register Asteroid *p;
    unsigned char i;
    for (i = 0, p = asteroids; i < MAX_ASTEROIDS; i++, p++) {
        if (p->drawn) {
            asteroid_draw_one(p, p->prev_x, p->prev_y);
            p->drawn = 0;
        }
        if (p->active) {
            asteroid_draw_one(p, p->x, p->y);
            p->prev_x = p->x;
            p->prev_y = p->y;
            p->drawn  = 1;
        }
    }
}

/* Vélocités 8.8 des fragments (tuning Phase 37) */
#define V_MAX_AST    576
#define V_MIN_AST    128

/* RNG signé en 8.8 — port de SetAstVel ($7203) : AND #$8F. */
static int rand_offset(void)
{
    signed char s = (signed char)(rng8() & 0x8F);
    if (s < 0) s |= (signed char)0xF0;
    return ((int)s) << 6;
}

static int clamp_vel(int v)
{
    if (v >  V_MAX_AST)          v =  V_MAX_AST;
    if (v < -V_MAX_AST)          v = -V_MAX_AST;
    if (v > 0 && v <  V_MIN_AST) v =  V_MIN_AST;
    if (v < 0 && v > -V_MIN_AST) v = -V_MIN_AST;
    return v;
}

/* Fragmentation arcade — port de BreakAsteroid ($75EC) + SplitAsteroid
 * ($761D) + SetAstVel ($7203) : parent réduit + 2 enfants. */
void asteroids_fragment(unsigned char idx)
{
    register Asteroid *p = &asteroids[idx];
    register Asteroid *q;
    unsigned char child_size;
    unsigned char i, found, sh;
    int vx_p, vy_p;
    int ax, ay;

    if (!p->active) return;

    /* Phase 10k : 80 frames arcade à 60 Hz ≈ 1,33 s → 40 frames à 30 Hz */
    ast_break_timer = 40;

    if (p->size == SIZE_SMALL) {
        p->active = 0;
        return;
    }

    /* Effacer le tracé OLD avant de changer size/shape (invariant erase). */
    if (p->drawn) {
        asteroid_draw_one(p, p->prev_x, p->prev_y);
        p->drawn = 0;
    }

    ax   = p->x;
    ay   = p->y;
    vx_p = p->vx;
    vy_p = p->vy;
    sh   = p->shape;
    child_size = p->size - 1;

    p->size  = child_size;
    p->shape = (sh + 1) & 3;
    p->vx    = clamp_vel(vx_p + rand_offset());
    p->vy    = clamp_vel(vy_p + rand_offset());

    found = 0;
    for (i = 0, q = asteroids; i < MAX_ASTEROIDS && found < 2; i++, q++) {
        if (q == p || q->active) continue;
        q->active = 1;
        q->drawn  = 0;
        q->size   = child_size;
        q->shape  = (sh + 2 + found) & 3;
        q->x      = ax;
        q->y      = ay;
        q->prev_x = ax;
        q->prev_y = ay;
        q->x_frac = 0;
        q->y_frac = 0;
        q->vx     = clamp_vel(vx_p + rand_offset());
        q->vy     = clamp_vel(vy_p + rand_offset());
        found++;
    }
}

unsigned char asteroids_count(void)
{
    register const Asteroid *p;
    unsigned char i, n = 0;
    for (i = 0, p = asteroids; i < MAX_ASTEROIDS; i++, p++) if (p->active) n++;
    return n;
}
