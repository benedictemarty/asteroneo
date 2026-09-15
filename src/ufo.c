/*
 * ufo.c — Soucoupe Phase 6 (port Neo6502)
 *
 * Une seule soucoupe simultanée : apparition sur un bord (RNG), traversée
 * horizontale avec inflexions verticales, disparition à l'autre bord.
 * Forme en 7 segments (corps trapézoïdal + dôme + ligne médiane), 2
 * tailles. IA : grande = tir aléatoire 8 directions ; petite = visée
 * approximative du ship, précision indexée sur le score (arcade $6CA5+).
 */

#include "ufo.h"
#include "asteroids.h"      /* rng8, scr_speedup, asteroids_count */
#include "line.h"
#include "sound.h"
#include "screen.h"

unsigned char ufo_active;
int           ufo_x, ufo_y;
signed char   ufo_vx, ufo_vy;
unsigned char ufo_type;
unsigned char ufo_bullet_active;
int           ufo_bullet_x, ufo_bullet_y;
signed char   ufo_bullet_vx, ufo_bullet_vy;
unsigned char ufo_bullet_ttl;

static unsigned int  ufo_spawn_timer;
static unsigned char ufo_fire_timer;
static unsigned char ufo_drift_timer;

/* Phase 36 — état écran du tir, découplé de ufo_bullet_active. */
static unsigned char ufo_blt_drawn;
static int           ufo_blt_px, ufo_blt_py;

#define UFO_BULLET_TTL      36    /* 30 frames à 25 Hz → 36 à 30 Hz */
#define UFO_BULLET_SPEED    4
#define UFO_FIRE_PERIOD     42    /* ~1,4 s */
#define UFO_SPAWN_PERIOD    360U  /* ~12 s normal */
#define UFO_SPAWN_FAST      144U  /* ~5 s quand asteroids_count ≤ ScrSpeedup */
#define UFO_DRIFT_PERIOD    24

/* Bande verticale de vol (sous le HUD) */
#define UFO_Y_MIN   30
#define UFO_Y_MAX   (SCR_H - 30)

#define N_SEGS 7
static const signed char seg_large[N_SEGS][4] = {
    { -8, -1,  +8, -1 },        /* ligne médiane */
    { -4, +3,  +4, +3 },        /* base ouverte */
    { -8, -1,  -4, +3 },        /* corps */
    {  4, +3,  +8, -1 },
    { -4, -4,  +4, -4 },        /* dôme */
    { -4, -4,  -2, -1 },
    {  4, -4,  +2, -1 },
};

static const signed char seg_small[N_SEGS][4] = {
    { -4, -1,  +4, -1 },
    { -2, +2,  +2, +2 },
    { -4, -1,  -2, +2 },
    {  2, +2,  +4, -1 },
    { -2, -2,  +2, -2 },
    { -2, -2,   0, -1 },
    {  2, -2,   0, -1 },
};

static void line(int x0, int y0, int x1, int y1)
{
    lx0 = x0; ly0 = y0; lx1 = x1; ly1 = y1;
    draw_line_xor();
}

unsigned char ufo_radius(void)
{
    return (ufo_type == UFO_LARGE) ? 8 : 4;
}

void ufo_init(void)
{
    ufo_active = 0;
    ufo_bullet_active = 0;
    ufo_spawn_timer = UFO_SPAWN_PERIOD;
    ufo_fire_timer = UFO_FIRE_PERIOD;
    ufo_drift_timer = UFO_DRIFT_PERIOD;
}

void ufo_kill(void)
{
    ufo_active = 0;
    ufo_bullet_active = 0;
    sound_stop_ufo();
}

static void ufo_spawn(unsigned int score_in)
{
    unsigned char r;
    /* Probabilité de small UFO croissante avec le score (cf. arcade). */
    r = rng8();
    if (score_in >= 10000U)      ufo_type = (r < 128) ? UFO_SMALL : UFO_LARGE;
    else if (score_in >= 5000U)  ufo_type = (r <  77) ? UFO_SMALL : UFO_LARGE;
    else                         ufo_type = UFO_LARGE;

    r = rng8();
    if (r & 1) {
        ufo_x = 9;
        ufo_vx = 1;
    } else {
        ufo_x = SCR_W - 10;
        ufo_vx = -1;
    }
    ufo_y = UFO_Y_MIN + (rng8() % (UFO_Y_MAX - UFO_Y_MIN));
    ufo_vy = 0;
    ufo_active = 1;
    ufo_fire_timer = UFO_FIRE_PERIOD;
    ufo_drift_timer = UFO_DRIFT_PERIOD;
    sound_play_fx(ufo_type == UFO_LARGE ? FX_UFO : FX_UFO_SMALL);
}

static void ufo_fire(int ship_x_in, int ship_y_in, unsigned int score_in)
{
    signed char dx, dy;
    unsigned char r;
    if (ufo_bullet_active) return;
    if (ufo_type == UFO_LARGE) {
        r = rng8() & 7;
        switch (r) {
            case 0: dx = +UFO_BULLET_SPEED; dy = 0; break;
            case 1: dx = +UFO_BULLET_SPEED; dy = +UFO_BULLET_SPEED; break;
            case 2: dx = 0;                 dy = +UFO_BULLET_SPEED; break;
            case 3: dx = -UFO_BULLET_SPEED; dy = +UFO_BULLET_SPEED; break;
            case 4: dx = -UFO_BULLET_SPEED; dy = 0; break;
            case 5: dx = -UFO_BULLET_SPEED; dy = -UFO_BULLET_SPEED; break;
            case 6: dx = 0;                 dy = -UFO_BULLET_SPEED; break;
            default: dx = +UFO_BULLET_SPEED; dy = -UFO_BULLET_SPEED; break;
        }
    } else {
        /* Visée approximative (port CalcScrShotDir / ScrShotAddOffset). */
        int ddx = ship_x_in - ufo_x;
        int ddy = ship_y_in - ufo_y;
        if (ddx > 4)       dx = +UFO_BULLET_SPEED;
        else if (ddx < -4) dx = -UFO_BULLET_SPEED;
        else               dx = 0;
        if (ddy > 4)       dy = +UFO_BULLET_SPEED;
        else if (ddy < -4) dy = -UFO_BULLET_SPEED;
        else               dy = 0;
        /* Précision indexée sur le score (arcade $6CB1 : seuil 35 000). */
        if (score_in < 35000U) {
            if (rng8() & 1) {
                r = rng8() & 1;
                if (r) dx = -dx;
                else   dy = -dy;
            }
        } else {
            if ((rng8() & 7) == 0) {
                r = rng8() & 1;
                if (r) dx = -dx;
                else   dy = -dy;
            }
        }
        if (dx == 0 && dy == 0) dy = +UFO_BULLET_SPEED;
    }
    ufo_bullet_x = ufo_x;
    ufo_bullet_y = ufo_y;
    ufo_bullet_vx = dx;
    ufo_bullet_vy = dy;
    ufo_bullet_ttl = UFO_BULLET_TTL;
    ufo_bullet_active = 1;
}

void ufo_tick(int ship_x_in, int ship_y_in, unsigned int score_in)
{
    int nx;
    if (!ufo_active) {
        if (ast_break_timer) ast_break_timer--;

        if (ufo_spawn_timer == 0) {
            /* Conditions arcade (UpdateScr $6BBF-$6BC9) : pas de spawn si
             * un asteroid vient d'être détruit ou si la vague est vide. */
            if (ast_break_timer == 0 && asteroids_count() != 0) {
                ufo_spawn(score_in);
                if (asteroids_count() <= scr_speedup) {
                    ufo_spawn_timer = UFO_SPAWN_FAST;
                } else {
                    ufo_spawn_timer = UFO_SPAWN_PERIOD;
                }
            }
        } else {
            ufo_spawn_timer--;
        }
        return;
    }
    nx = ufo_x + ufo_vx;
    if (nx < 9 || nx > SCR_W - 10) {
        ufo_active = 0;
        sound_stop_ufo();
        return;
    }
    ufo_x = nx;
    if (ufo_drift_timer == 0) {
        ufo_drift_timer = UFO_DRIFT_PERIOD;
        switch (rng8() & 3) {
            case 0:  ufo_vy = -1; break;
            case 1:  ufo_vy =  0; break;
            default: ufo_vy = +1; break;
        }
    } else {
        ufo_drift_timer--;
    }
    if (ufo_vy) {
        nx = ufo_y + ufo_vy;
        if (nx >= UFO_Y_MIN && nx <= UFO_Y_MAX) ufo_y = nx;
    }
    if (ufo_fire_timer == 0) {
        ufo_fire(ship_x_in, ship_y_in, score_in);
        ufo_fire_timer = UFO_FIRE_PERIOD;
    } else {
        ufo_fire_timer--;
    }
}

void ufo_draw(void)
{
    unsigned char i;
    const signed char (*segs)[4];
    /* Pas de test ufo_active : permet l'erase après ufo_kill (ufo_x/y/type
     * préservés). L'appelant tient un flag ufo_was_drawn. */
    segs = (ufo_type == UFO_LARGE) ? seg_large : seg_small;
    for (i = 0; i < N_SEGS; i++) {
        line(ufo_x + segs[i][0], ufo_y + segs[i][1],
             ufo_x + segs[i][2], ufo_y + segs[i][3]);
    }
}

void ufo_bullet_update(void)
{
    int nx, ny;
    if (!ufo_bullet_active) return;
    if (ufo_bullet_ttl == 0) {
        ufo_bullet_active = 0;
        return;
    }
    nx = ufo_bullet_x + ufo_bullet_vx;
    ny = ufo_bullet_y + ufo_bullet_vy;
    /* Bloc 2×2 px : x ∈ [0..SCR_W-2], y ∈ [0..SCR_H-2]. */
    if (nx < 0)               nx += SCR_W - 1;
    else if (nx > SCR_W - 2)  nx -= SCR_W - 1;
    if (ny < 0)               ny += SCR_H - 1;
    else if (ny > SCR_H - 2)  ny -= SCR_H - 1;
    ufo_bullet_x = nx;
    ufo_bullet_y = ny;
    ufo_bullet_ttl--;
}

static void ufo_dot4(int x, int y)
{
    lx0 = x;     ly0 = y;     plot_dot();
    lx0 = x + 1;              plot_dot();
    ly0 = y + 1;              plot_dot();
    lx0 = x;                  plot_dot();
}

void ufo_bullet_commit(void)
{
    /* Erase à la position du dernier draw puis draw à la position
     * courante (Phase 36) : jamais de pixel fantôme. */
    if (ufo_blt_drawn) {
        ufo_dot4(ufo_blt_px, ufo_blt_py);
        ufo_blt_drawn = 0;
    }
    if (ufo_bullet_active) {
        ufo_dot4(ufo_bullet_x, ufo_bullet_y);
        ufo_blt_px = ufo_bullet_x;
        ufo_blt_py = ufo_bullet_y;
        ufo_blt_drawn = 1;
    }
}
