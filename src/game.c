/*
 * game.c — Boucle de jeu Astéroric (port Neo6502)
 *
 * Phase 3 : cadence, input clavier, ship physics, 4 tirs.
 * Phase 4 : asteroids (spawn, draw, update, fragment).
 * Phase 5 : collisions (L∞), score 7-segments, vies, respawn invincible.
 * Phase 6 : UFO grande/petite + IA tir + collisions UFO.
 * Phase 7 : hyperespace (DOWN), game state PLAY/GAMEOVER + restart,
 *           high scores top 5 affiché en game over.
 *
 * Port Neo6502 : coordonnées int sur 320 × 240, cadence 30 Hz (2 trames
 * de 60 Hz par pas de jeu, cf. frame_wait), état du ship en C (ship.s
 * supprimé), primitives XOR servies par l'API graphique (neo_gfx.s).
 */

#include "asteroids.h"
#include "hud.h"
#include "ufo.h"
#include "sound.h"
#include "title.h"
#include "keys.h"
#include "line.h"
#include "phys.h"
#include "screen.h"
#include "neo_time.h"

/* ------------------------------------------------------------------ */
/* État du vaisseau (ex-ship.s)                                        */
/* ------------------------------------------------------------------ */

static int           ship_x, ship_y;
static unsigned char ship_x_frac, ship_y_frac;   /* 8.8 partie basse */
static int           ship_vx, ship_vy;           /* 8.8 signé */
static unsigned char ship_angle;                 /* 0..31 */

/* ship_verts.c (généré) */
extern const signed char ship_thrx[32];
extern const signed char ship_thry[32];
extern const signed char ship_pt0x[32], ship_pt0y[32];
extern const signed char ship_pt1x[32], ship_pt1y[32];
extern const signed char ship_pt2x[32], ship_pt2y[32];
extern const signed char ship_pt3x[32], ship_pt3y[32];
extern const signed char ship_pt4x[32], ship_pt4y[32];

static void ship_init(void)
{
    ship_x = SCR_HALF_W;
    ship_y = SCR_HALF_H;
    ship_x_frac = 0;
    ship_y_frac = 0;
    ship_vx = 0;
    ship_vy = 0;
    ship_angle = 0;
    key_state = 0;
}

static void ship_rotate(signed char delta)
{
    ship_angle = (unsigned char)(ship_angle + delta) & 0x1F;
}

/* ------------------------------------------------------------------ */
/* Constantes                                                          */
/* ------------------------------------------------------------------ */

#define BULLETS         4
/* TTL bullet : 18 frames arcade à 60 Hz ; 15 frames à 25 Hz sur Oric
 * (≈ 0,6 s, portée ≈ 75 % de l'écran) → 18 frames à 30 Hz. */
#define BULLET_TTL      18
#define FIRE_COOLDOWN   2
/* Torpille = bloc 2×2 px : x ∈ [0..SCR_W-2], y ∈ [0..SCR_H-2]. */
#define BLT_X_MIN       0
#define BLT_X_MAX       (SCR_W - 2)
#define BLT_Y_MIN       0
#define BLT_Y_MAX       (SCR_H - 2)
#define BLT_X_SPAN      (BLT_X_MAX - BLT_X_MIN + 1)
#define BLT_Y_SPAN      (BLT_Y_MAX - BLT_Y_MIN + 1)

/* 8.8 fixed-point pour ship_vx/vy : V_MAX = 8 px/frame, thrust ×64,
 * decay 1/16 par frame (équilibre arcade conservé). */
#define V_MAX_FIXED     2048
#define THRUST_SHIFT    6

/* Zone de téléportation hyperespace */
#define WX_MIN          20
#define WX_MAX          (SCR_W - 20)
#define WY_MIN          20
#define WY_MAX          (SCR_H - 20)
#define WX_SPAN         (WX_MAX - WX_MIN)
#define WY_SPAN         (WY_MAX - WY_MIN)

/* Demi-extent L∞ du ship (apex 6 px) : hitbox = silhouette. */
#define SHIP_RADIUS         6
#define SHIP_BLINK_FRAMES   24
#define INVINCIBLE_FRAMES   (DEBRIS_TTL + SHIP_BLINK_FRAMES)

/* Phase 7 — hyperespace */
#define HYPER_COOLDOWN      42      /* ~1,4 s, edge-trigger sur DOWN */
#define HYPER_DEATH_CHANCE  64      /* 64/256 = 25 % */

#define HISCORE_COUNT       5

/* Phase 8 — cadence du thump (frames à 30 Hz) */
#define THUMP_PERIOD_BASE   36
#define THUMP_PERIOD_MIN    7

/* Phase 33 — jingle titre : « Dans l'antre du roi de la montagne »
 * (Grieg, Peer Gynt, 1875 — domaine public), la mineur, 3 énoncés en
 * accelerando puis A4 tenu. Durées en frames de jeu. */
static const unsigned char title_tune_note[] = {
     9, 11, 12, 14, 16, 12, 16,   15, 11, 15,   14, 10, 14,
    16, 18, 19, 21, 23, 19, 23,   22, 18, 22,   21, 17, 21,
    16, 18, 19, 21, 23, 19, 23,   22, 18, 22,   21, 17, 21,
    21
};
static const unsigned char title_tune_dur[] = {
     5, 5, 5, 5, 5, 5, 10,   5, 5, 10,   5, 5, 10,
     4, 4, 4, 4, 4, 4, 8,    4, 4, 8,    4, 4, 8,
     2, 2, 2, 2, 2, 2, 4,    2, 2, 4,    2, 2, 4,
    16
};
#define TITLE_TUNE_LEN  40

static const unsigned int score_by_size[3] = { 100, 50, 20 };
#define UFO_SCORE_LARGE     200U
#define UFO_SCORE_SMALL     1000U

/* Cadence : 60 Hz écran / 2 = 30 Hz de jeu. */
#define VSYNCS_PER_FRAME 2

/* ------------------------------------------------------------------ */
/* État local                                                          */
/* ------------------------------------------------------------------ */

static int           blt_x[BULLETS];
static int           blt_y[BULLETS];
static signed char   blt_vx[BULLETS];
static signed char   blt_vy[BULLETS];
static unsigned char blt_ttl[BULLETS];
/* Phase 36 — état écran des torpilles : bitmask des slots XOR-és + position
 * du dernier draw (erase toujours exactement ce qui a été dessiné). */
static unsigned char blt_drawn;
static int           blt_px[BULLETS];
static int           blt_py[BULLETS];

static unsigned char fire_cd;
static unsigned char prev_hyper;
static unsigned char hyper_cd;

static unsigned char ship_invincible;
static unsigned char ship_was_drawn;
static unsigned char ufo_was_drawn;
static unsigned char flame_was_drawn;
static unsigned char flame_flick;

static unsigned int  hiscores[HISCORE_COUNT];
static unsigned char hiscores_drawn;
static unsigned char gameover_text_drawn;
static unsigned char prompt_drawn;
static unsigned char gameover_elapsed;
static unsigned char gameover_armed;

static unsigned char thump_timer;
static unsigned char lives_prev;
static unsigned char wave_displayed;
#define WAVE_HUD_Y  16

/* Phase 12/13 — debris ship/UFO arcade (ShipExpVelTbl $50EC, ShipExpPtrTbl
 * $50E0) : 6 fragments, vélocités fixes, disparition séquentielle. */
#define DEBRIS_COUNT      6
#define DEBRIS_TTL        48      /* 1,6 s à 30 Hz */

static const signed char ship_debris_vx[DEBRIS_COUNT] = { -3, +3,  0, +3,  0, -3 };
static const signed char ship_debris_vy[DEBRIS_COUNT] = { +1, -2, -4, +1, +4, -3 };
static const signed char ship_debris_shape_dx[DEBRIS_COUNT] = { -2, +1, +3, -1, -3, +1 };
static const signed char ship_debris_shape_dy[DEBRIS_COUNT] = { -3, -2, +1, +1, +1, -1 };

static int            dbr_x[DEBRIS_COUNT];
static int            dbr_y[DEBRIS_COUNT];
static signed char    dbr_vx[DEBRIS_COUNT];
static signed char    dbr_vy[DEBRIS_COUNT];
static unsigned char  dbr_ttl[DEBRIS_COUNT];

/* Phase 14b — explosion asteroid : pattern shrapnel 1 arcade ($5100), 8
 * dots fixes autour du centre, flash très bref. */
#define ADBR_COUNT  8
#define ADBR_TTL    2
static int            adbr_x[ADBR_COUNT];
static int            adbr_y[ADBR_COUNT];
static unsigned char  adbr_ttl[ADBR_COUNT];
static const signed char adbr_dx[ADBR_COUNT] = { -1, -2, -1, +2, +4, +4, +5, +4 };
static const signed char adbr_dy[ADBR_COUNT] = {  0, -1, -2, -1, -2, -1, +2, +5 };

/* Séquencement de l'écran game over (frames à 30 Hz) :
 *   [0, DEATH_EXPLOSION_END[ : explosion debris seule ;
 *   [.., DEATH_HOF_FRAME[    : + GAME OVER seul (5 s) ;
 *   [DEATH_HOF_FRAME, ∞[     : GAME OVER effacé, + HoF + prompt (après
 *                              relâchement SPACE/ESC). */
#define DEATH_EXPLOSION_END     48
#define DEATH_GAMEOVER_HOLD     150
#define DEATH_HOF_FRAME         (DEATH_EXPLOSION_END + DEATH_GAMEOVER_HOLD)

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void plot(int x, int y)
{
    lx0 = x; ly0 = y;
    plot_dot();
}

/* Bloc 2×2 px en XOR — torpilles. x ≤ SCR_W-2, y ≤ SCR_H-2. */
static void plot4(int x, int y)
{
    plot(x,     y);
    plot(x + 1, y);
    plot(x,     y + 1);
    plot(x + 1, y + 1);
}

/* ------------------------------------------------------------------ */
/* Cadence                                                             */
/* ------------------------------------------------------------------ */

/* Phase 28 (P5) — ordonnanceur à pas fixe : la cible avance de
 * VSYNCS_PER_FRAME trames par pas, indépendamment de la durée du
 * travail ; un pas en retard ne rattrape que le retard. Le compteur
 * de trames vient de l'API (5,37), octet bas modulo 256. */
static unsigned char frame_target;

/* Compteurs de diagnostic (lus par les tests : dump RAM Phosphoneo) —
 * pas de jeu exécutés et pas en retard d'au moins une trame. */
unsigned int dbg_frames;
unsigned int dbg_late;

static void frame_wait(void)
{
    unsigned char d, now;
    frame_target += VSYNCS_PER_FRAME;
    for (;;) {
        now = frame_tick();
        d = (unsigned char)(now - frame_target);
        if (d < 128) break;
    }
    if (d >= 1) dbg_late++;
    if (d >= VSYNCS_PER_FRAME) frame_target = now;
    dbg_frames++;
    sound_tick();
}

/* ------------------------------------------------------------------ */
/* Ship render — Phase 27 (G2/G3)                                      */
/* ------------------------------------------------------------------ */

#define SHIP_EXTENT  8

static void ship_seg_clip(int x0, int y0, int x1, int y1)
{
    if (x0 < 0 || x0 > SCR_XMAX || y0 < 0 || y0 > SCR_YMAX) return;
    if (x1 < 0 || x1 > SCR_XMAX || y1 < 0 || y1 > SCR_YMAX) return;
    lx0 = x0; ly0 = y0; lx1 = x1; ly1 = y1;
    draw_line_xor_open();
}

/* Une instance du ship centrée en (ox, oy). Orientation in-degree 1 :
 * chaque sommet est l'arrivée d'exactement un segment → peint 1×.
 * Flamme : 2 segments P3→F et P4→F (F XOR-é 2×, tip éteint). */
static void ship_render_at(int ox, int oy, unsigned char flame)
{
    unsigned char a = ship_angle;
    int x0 = ox + ship_pt0x[a], y0 = oy + ship_pt0y[a];
    int x1 = ox + ship_pt1x[a], y1 = oy + ship_pt1y[a];
    int x2 = ox + ship_pt2x[a], y2 = oy + ship_pt2y[a];
    int x3 = ox + ship_pt3x[a], y3 = oy + ship_pt3y[a];
    int x4 = ox + ship_pt4x[a], y4 = oy + ship_pt4y[a];

    ship_seg_clip(x3, y3, x0, y0);      /* peint P0 (apex) */
    ship_seg_clip(x3, y3, x1, y1);      /* peint P1 */
    ship_seg_clip(x0, y0, x4, y4);      /* peint P4 */
    ship_seg_clip(x4, y4, x2, y2);      /* peint P2 */
    ship_seg_clip(x4, y4, x3, y3);      /* peint P3 */
    if (flame) {
        int fx = ox - ship_thrx[a];
        int fy = oy - ship_thry[a];
        ship_seg_clip(x3, y3, fx, fy);
        ship_seg_clip(x4, y4, fx, fy);
    }
}

/* XOR le ship (+ flamme) à l'état courant. Idempotent : rappeler avec le
 * MÊME flag flamme pour effacer. Près d'un bord, instances fantômes à
 * ±SCR_W / ±SCR_H (coins : 4). */
static void ship_render(unsigned char flame)
{
    int dup_x = 0, dup_y = 0;
    if (ship_x < SHIP_EXTENT)              dup_x = +SCR_W;
    else if (ship_x > SCR_XMAX - SHIP_EXTENT) dup_x = -SCR_W;
    if (ship_y < SHIP_EXTENT)              dup_y = +SCR_H;
    else if (ship_y > SCR_YMAX - SHIP_EXTENT) dup_y = -SCR_H;
    ship_render_at(ship_x, ship_y, flame);
    if (dup_x) ship_render_at(ship_x + dup_x, ship_y, flame);
    if (dup_y) ship_render_at(ship_x, ship_y + dup_y, flame);
    if (dup_x && dup_y) ship_render_at(ship_x + dup_x, ship_y + dup_y, flame);
}

/* ------------------------------------------------------------------ */
/* Ship physics + hyperespace                                          */
/* ------------------------------------------------------------------ */

static void ship_update(void)
{
    int d;

    /* Decay (frottement) — 1/16 par frame en 8.8. */
    d = ship_vx >> 4;  ship_vx -= d;
    d = ship_vy >> 4;  ship_vy -= d;

    if (ship_vx >  V_MAX_FIXED) ship_vx =  V_MAX_FIXED;
    if (ship_vx < -V_MAX_FIXED) ship_vx = -V_MAX_FIXED;
    if (ship_vy >  V_MAX_FIXED) ship_vy =  V_MAX_FIXED;
    if (ship_vy < -V_MAX_FIXED) ship_vy = -V_MAX_FIXED;

    integrate(&ship_x, &ship_x_frac, ship_vx, SCR_W);
    integrate(&ship_y, &ship_y_frac, ship_vy, SCR_H);
}

static void ship_apply_thrust(void)
{
    ship_vx += ((int)ship_thrx[ship_angle]) << THRUST_SHIFT;
    ship_vy += ((int)ship_thry[ship_angle]) << THRUST_SHIFT;
}

static void ship_respawn(void)
{
    ship_x = SCR_HALF_W;
    ship_y = SCR_HALF_H;
    ship_x_frac = 0;
    ship_y_frac = 0;
    ship_vx = 0;
    ship_vy = 0;
    ship_angle = 0;
    ship_invincible = INVINCIBLE_FRAMES;
}

static void debris_spawn(int ax, int ay)
{
    unsigned char i;
    for (i = 0; i < DEBRIS_COUNT; i++) {
        dbr_x[i]   = ax;
        dbr_y[i]   = ay;
        dbr_vx[i]  = ship_debris_vx[i];
        dbr_vy[i]  = ship_debris_vy[i];
        dbr_ttl[i] = DEBRIS_TTL - i * 6;      /* 48/42/36/30/24/18 séquentielle */
    }
}

static void debris_update(void)
{
    unsigned char i;
    int nx, ny;
    for (i = 0; i < DEBRIS_COUNT; i++) {
        if (dbr_ttl[i] == 0) continue;
        nx = dbr_x[i] + dbr_vx[i];
        ny = dbr_y[i] + dbr_vy[i];
        if (nx < 4 || nx > SCR_XMAX - 4 || ny < 4 || ny > SCR_YMAX - 4) {
            dbr_ttl[i] = 0;
            continue;
        }
        dbr_x[i] = nx;
        dbr_y[i] = ny;
        dbr_ttl[i]--;
    }
}

/* Phase 13 : chaque debris est un mini-segment SVEC arcade. */
static void debris_render(void)
{
    unsigned char i;
    for (i = 0; i < DEBRIS_COUNT; i++) {
        if (dbr_ttl[i] == 0) continue;
        lx0 = dbr_x[i];
        ly0 = dbr_y[i];
        lx1 = dbr_x[i] + ship_debris_shape_dx[i];
        ly1 = dbr_y[i] + ship_debris_shape_dy[i];
        draw_line_xor();
    }
}

static void debris_init(void)
{
    unsigned char i;
    for (i = 0; i < DEBRIS_COUNT; i++) dbr_ttl[i] = 0;
    for (i = 0; i < ADBR_COUNT; i++) adbr_ttl[i] = 0;
}

static void asteroid_debris_spawn(int ax, int ay)
{
    unsigned char i;
    for (i = 0; i < ADBR_COUNT; i++) {
        adbr_x[i]   = ax;
        adbr_y[i]   = ay;
        adbr_ttl[i] = ADBR_TTL;
    }
}

static void asteroid_debris_render(void)
{
    unsigned char i;
    int x, y;
    for (i = 0; i < ADBR_COUNT; i++) {
        if (adbr_ttl[i] == 0) continue;
        x = adbr_x[i] + adbr_dx[i];
        y = adbr_y[i] + adbr_dy[i];
        if (x < 0 || x > SCR_XMAX || y < 0 || y > SCR_YMAX) continue;
        lx0 = x;
        ly0 = y;
        plot_dot();
    }
}

static void asteroid_debris_update(void)
{
    unsigned char i;
    for (i = 0; i < ADBR_COUNT; i++) {
        if (adbr_ttl[i]) adbr_ttl[i]--;
    }
}

/* Hyperespace : téléportation aléatoire, 25 % de chance de mort. */
static void ship_hyperspace(void)
{
    sound_play_fx(FX_HYPER);
    if (rng8() < HYPER_DEATH_CHANCE) {
        hud_lose_life();
        ship_respawn();
        return;
    }
    ship_x = WX_MIN + ((rng8() | ((int)rng8() << 8)) % WX_SPAN);
    ship_y = WY_MIN + (rng8() % WY_SPAN);
    ship_x_frac = 0;
    ship_y_frac = 0;
    ship_vx = 0;
    ship_vy = 0;
    ship_invincible = SHIP_BLINK_FRAMES;
}

/* ------------------------------------------------------------------ */
/* Bullets                                                            */
/* ------------------------------------------------------------------ */

static void bullets_init(void)
{
    unsigned char i;
    for (i = 0; i < BULLETS; i++) blt_ttl[i] = 0;
    fire_cd = 0;
    prev_hyper = 0;
    hyper_cd = 0;
}

static void bullet_fire(void)
{
    unsigned char i;
    if (fire_cd) return;
    for (i = 0; i < BULLETS; i++) {
        if (blt_ttl[i] == 0) {
            blt_x[i]   = ship_x;
            blt_y[i]   = ship_y;
            /* Vitesse bullet = 2× le thrust ship (~12 px/frame max). */
            blt_vx[i]  = (signed char)(ship_thrx[ship_angle] << 1);
            blt_vy[i]  = (signed char)(ship_thry[ship_angle] << 1);
            blt_ttl[i] = BULLET_TTL;
            fire_cd    = FIRE_COOLDOWN;
            sound_play_fx(FX_FIRE);
            return;
        }
    }
}

static void bullets_update(void)
{
    unsigned char i;
    int nx, ny;
    if (fire_cd) fire_cd--;
    for (i = 0; i < BULLETS; i++) {
        if (blt_ttl[i] == 0) continue;
        nx = blt_x[i] + blt_vx[i];
        ny = blt_y[i] + blt_vy[i];
        if (nx < BLT_X_MIN)      nx += BLT_X_SPAN;
        else if (nx > BLT_X_MAX) nx -= BLT_X_SPAN;
        if (ny < BLT_Y_MIN)      ny += BLT_Y_SPAN;
        else if (ny > BLT_Y_MAX) ny -= BLT_Y_SPAN;
        blt_x[i] = nx;
        blt_y[i] = ny;
        blt_ttl[i]--;
    }
}

/* Phase 36 — erase à la position du dernier draw immédiatement suivi du
 * draw à la position courante (fenêtre d'absence minimale). */
static void bullets_commit(void)
{
    unsigned char i, m;
    for (i = 0, m = 1; i < BULLETS; i++, m <<= 1) {
        if (blt_drawn & m) {
            plot4(blt_px[i], blt_py[i]);            /* erase */
            blt_drawn &= (unsigned char)~m;
        }
        if (blt_ttl[i]) {
            plot4(blt_x[i], blt_y[i]);              /* draw */
            blt_px[i] = blt_x[i];
            blt_py[i] = blt_y[i];
            blt_drawn |= m;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Collisions                                                         */
/* ------------------------------------------------------------------ */

static void collisions_bullets_asteroids(void)
{
    register Asteroid *p;
    unsigned char b, a, r;
    for (b = 0; b < BULLETS; b++) {
        if (blt_ttl[b] == 0) continue;
        if (ufo_active) {
            r = ufo_radius() + 1;
            if (collide(blt_x[b], blt_y[b], ufo_x, ufo_y, r)) {
                hud_add_score((ufo_type == UFO_LARGE) ? UFO_SCORE_LARGE
                                                       : UFO_SCORE_SMALL);
                debris_spawn(ufo_x, ufo_y);
                sound_play_fx(FX_EXPLODE);
                ufo_kill();
                blt_ttl[b] = 0;
                continue;
            }
        }
        for (a = 0, p = asteroids; a < MAX_ASTEROIDS; a++, p++) {
            unsigned char sz;
            if (!p->active) continue;
            r = shape_radii[p->size] + 1;
            if (collide(blt_x[b], blt_y[b], p->x, p->y, r)) {
                sz = p->size;
                hud_add_score(score_by_size[sz]);
                asteroid_debris_spawn(p->x, p->y);
                asteroids_fragment(a);
                sound_play_fx(sz == SIZE_LARGE  ? FX_EXPLODE
                            : sz == SIZE_MEDIUM ? FX_BANG_MEDIUM
                                                : FX_BANG_SMALL);
                blt_ttl[b] = 0;
                break;
            }
        }
    }
}

static unsigned char collisions_ship_asteroids(void)
{
    register Asteroid *p;
    unsigned char a, r;
    if (ship_invincible) return 0;
    for (a = 0, p = asteroids; a < MAX_ASTEROIDS; a++, p++) {
        if (!p->active) continue;
        r = shape_radii[p->size] + SHIP_RADIUS;
        if (collide(ship_x, ship_y, p->x, p->y, r)) {
            hud_lose_life();
            debris_spawn(ship_x, ship_y);
            asteroid_debris_spawn(p->x, p->y);
            ship_respawn();
            sound_play_fx(FX_EXPLODE);
            return 1;
        }
    }
    if (ufo_active) {
        r = ufo_radius() + SHIP_RADIUS;
        if (collide(ship_x, ship_y, ufo_x, ufo_y, r)) {
            hud_lose_life();
            debris_spawn(ship_x, ship_y);
            debris_spawn(ufo_x, ufo_y);
            sound_play_fx(FX_EXPLODE);
            ship_respawn();
            ufo_kill();
            return 1;
        }
    }
    if (ufo_bullet_active) {
        if (collide(ship_x, ship_y, ufo_bullet_x, ufo_bullet_y, SHIP_RADIUS)) {
            hud_lose_life();
            debris_spawn(ship_x, ship_y);
            sound_play_fx(FX_EXPLODE);
            ship_respawn();
            ufo_bullet_active = 0;
            return 1;
        }
    }
    return 0;
}

static void collisions_ufobullet_asteroids(void)
{
    register Asteroid *p;
    unsigned char a, r;
    if (!ufo_bullet_active) return;
    for (a = 0, p = asteroids; a < MAX_ASTEROIDS; a++, p++) {
        unsigned char sz;
        if (!p->active) continue;
        r = shape_radii[p->size] + 1;
        if (collide(ufo_bullet_x, ufo_bullet_y, p->x, p->y, r)) {
            sz = p->size;
            asteroids_fragment(a);
            sound_play_fx(sz == SIZE_LARGE  ? FX_EXPLODE
                        : sz == SIZE_MEDIUM ? FX_BANG_MEDIUM
                                            : FX_BANG_SMALL);
            ufo_bullet_active = 0;
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Vagues                                                              */
/* ------------------------------------------------------------------ */

static void check_next_wave(void)
{
    if (asteroids_count() == 0) {
        asteroids_spawn_wave();
    }
}

/* ------------------------------------------------------------------ */
/* High scores (Phase 7)                                              */
/* ------------------------------------------------------------------ */

static void hiscores_init(void)
{
    hiscores[0] = 1000;
    hiscores[1] = 500;
    hiscores[2] = 200;
    hiscores[3] = 100;
    hiscores[4] = 50;
    hiscores_drawn = 0;
}

static unsigned char hiscores_insert(unsigned int final_score)
{
    unsigned char i, j;
    for (i = 0; i < HISCORE_COUNT; i++) {
        if (final_score > hiscores[i]) {
            for (j = HISCORE_COUNT - 1; j > i; j--) {
                hiscores[j] = hiscores[j - 1];
            }
            hiscores[i] = final_score;
            return i;
        }
    }
    return 0xFF;
}

#define HISCORES_LABEL_Y  66
static void hiscores_draw_table(void)
{
    unsigned char i;
    int py;
    hiscores_label_draw(HISCORES_LABEL_Y);
    for (i = 0; i < HISCORE_COUNT; i++) {
        py = 90 + i * 16;
        hud_xor_5digits(hiscores[i], SCR_HALF_W - 15, py);
    }
}

/* ------------------------------------------------------------------ */
/* Réinitialisation pour restart                                       */
/* ------------------------------------------------------------------ */

static void game_reset(void)
{
    if (ship_was_drawn) ship_render(flame_was_drawn);
    flame_was_drawn = 0;
    asteroids_draw();
    if (ufo_was_drawn) ufo_draw();
    ufo_was_drawn = 0;

    ship_init();
    bullets_init();
    asteroids_init(0x42);
    asteroids_spawn_wave();
    ufo_init();
    hud_erase();
    hud_init();
    bullets_commit();
    ufo_bullet_commit();
    ship_was_drawn  = 0;
    ship_invincible = 0;
    ship_render(0);
    ship_was_drawn = 1;
    asteroids_draw();
    hud_draw();
}

/* ------------------------------------------------------------------ */
/* Boucle principale                                                  */
/* ------------------------------------------------------------------ */

#define TITLE_PRESS_Y   130
#define TITLE_HINT_Y    166
#define GO_PRESS_Y      184
#define GO_QUIT_Y       202

void game_run(void)
{
    unsigned char hyper_now;
    unsigned char ship_visible;
    unsigned char prev_gameover;
    unsigned int  final_score;
    unsigned char new_hiscore_pos = 0xFF;

    ship_invincible     = 0;
    ship_was_drawn      = 0;
    ufo_was_drawn       = 0;
    flame_was_drawn     = 0;
    flame_flick         = 0;
    prev_gameover       = 0;
    final_score         = 0;
    hiscores_drawn      = 0;
    gameover_text_drawn = 0;
    prompt_drawn        = 0;
    gameover_elapsed    = 0;
    gameover_armed      = 0;

    gfx_init();
    sound_init();
    frame_target = frame_tick();

    /* Écran titre : "ASTERONEO", "PRESS SPACE" clignotant, démo passive. */
    title_draw();
    presspace_draw(TITLE_PRESS_Y);
    keyshint_draw(TITLE_HINT_Y);
    asteroids_init(0x42);
    asteroids_spawn_wave();
    asteroids_draw();
    {
        unsigned char i = 0;
        unsigned char prev_space = 0;
        unsigned char ps_visible = 1;
        unsigned char tune_pos = 0;
        unsigned char tune_frame = 0;
        for (;;) {
            key_scan();
            if ((key_state & KS_FIRE) && !prev_space) break;
            prev_space = key_state & KS_FIRE;
            asteroids_update();
            asteroids_render();
            key_scan();
            if ((key_state & KS_FIRE) && !prev_space) break;
            prev_space = key_state & KS_FIRE;
            /* K ouvre l'écran de config des touches. */
            if (key_probe() == KEY_PROBE_K) {
                tune_stop();
                if (ps_visible) presspace_erase(TITLE_PRESS_Y);
                keyshint_erase(TITLE_HINT_Y);
                title_erase();
                keys_screen();
                title_draw();
                keyshint_draw(TITLE_HINT_Y);
                presspace_draw(TITLE_PRESS_Y);
                ps_visible = 1;
                prev_space = 0;
            }
            /* Jingle : un pas par frame, legato, respiration en fin de phrase. */
            if (tune_pos < TITLE_TUNE_LEN) {
                if (tune_frame == 0)
                    tune_play_note(title_tune_note[tune_pos]);
                tune_frame++;
                if (title_tune_dur[tune_pos] >= 8 &&
                    tune_frame == title_tune_dur[tune_pos] - 1)
                    tune_stop();
                if (tune_frame >= title_tune_dur[tune_pos]) {
                    tune_frame = 0;
                    tune_pos++;
                }
            }
            /* Toggle PRESS SPACE toutes les 16 frames (~0,5 s). */
            if ((i & 0x0F) == 0x0F) {
                if (ps_visible) {
                    presspace_erase(TITLE_PRESS_Y);
                    ps_visible = 0;
                } else {
                    presspace_draw(TITLE_PRESS_Y);
                    ps_visible = 1;
                }
            }
            i++;
            frame_wait();
        }
        if (ps_visible) presspace_erase(TITLE_PRESS_Y);
        keyshint_erase(TITLE_HINT_Y);
        tune_stop();
    }
    title_erase();

    ship_init();
    bullets_init();
    ufo_init();
    hud_init();
    hiscores_init();
    debris_init();
    thump_timer = THUMP_PERIOD_BASE;
    lives_prev = lives;
    wave_displayed = 0;
    flame_was_drawn = 0;
    flame_flick = 0;

    ship_render(0);
    ship_was_drawn = 1;
    asteroids_draw();
    hud_draw();

    for (;;) {
        key_scan();

        /* Wait-release : ignorer SPACE/ESC maintenus depuis avant la mort. */
        if (gameover && gameover_elapsed >= DEATH_HOF_FRAME && gameover_armed) {
            if ((key_state & (KS_FIRE | KS_ESC)) == 0) {
                gameover_armed = 0;
            }
        }

        if (gameover && (gameover_elapsed < DEATH_HOF_FRAME || gameover_armed)) {
            /* Phase 1 ou 2, ou touches encore maintenues : ignore. */
        } else if (gameover) {
            if (key_state & KS_FIRE) {
                if (prompt_drawn) {
                    presspace_erase(GO_PRESS_Y);
                    quit_label_erase(GO_QUIT_Y);
                    prompt_drawn = 0;
                }
                if (gameover_text_drawn) {
                    gameover_erase();
                    gameover_text_drawn = 0;
                }
                if (hiscores_drawn) {
                    hiscores_draw_table();
                    hiscores_drawn = 0;
                }
                gameover_elapsed = 0;
                gameover_armed   = 0;
                game_reset();
                prev_gameover = 0;
                continue;
            }
            if (key_state & KS_ESC) {
                /* ESC → quitter : silence puis retour au noyau (crt0). */
                sound_mute();
                gfx_clear();
                return;
            }
        }

        /* ===== ERASE → LOGIQUE → DRAW (fenêtre flicker compactée) ===== */

        asteroid_debris_render();
        debris_render();

        if (!gameover) {
            if (key_state & KS_FIRE) bullet_fire();
        }

        bullets_update();
        ufo_bullet_update();
        debris_update();
        asteroid_debris_update();

        /* ===== ASTEROIDS — per-entity erase/draw ===== */
        asteroids_update();
        collisions_bullets_asteroids();
        collisions_ufobullet_asteroids();
        asteroids_render();

        /* ===== UFO — bloc compact erase → tick → draw ===== */
        if (ufo_was_drawn) ufo_draw();
        if (!gameover) ufo_tick(ship_x, ship_y, score);
        if (ufo_active) {
            ufo_draw();
            ufo_was_drawn = 1;
        } else {
            ufo_was_drawn = 0;
        }

        /* ===== SHIP — bloc compact erase → input → update → draw ===== */
        if (ship_was_drawn) ship_render(flame_was_drawn);

        if (!gameover) {
            /* Rotation : 1 pas de 11,25° par frame à 30 Hz → tour complet en ≈ 1,1 s
             * (2 pas venait de l'Oric à ~10-17 Hz effectifs : trop vif ici). */
            if (key_state & KS_LEFT)  ship_rotate((signed char)-1);
            if (key_state & KS_RIGHT) ship_rotate((signed char)+1);
            if (key_state & KS_THRUST) {
                ship_apply_thrust();
                if (sfx_id == FX_NONE) sound_play_fx(FX_THRUST);
            }
            hyper_now = key_state & KS_HYPER;
            if (hyper_now && !prev_hyper && hyper_cd == 0) {
                ship_hyperspace();
                hyper_cd = HYPER_COOLDOWN;
            }
            prev_hyper = hyper_now;
            if (hyper_cd) hyper_cd--;

            ship_update();
            collisions_ship_asteroids();
        }

        if (ship_invincible) ship_invincible--;
        check_next_wave();

        ship_visible = !gameover &&
                       (ship_invincible <= SHIP_BLINK_FRAMES) &&
                       (ship_invincible == 0 || (ship_invincible & 2));

        if (ship_visible) {
            unsigned char flame_now;
            flame_flick++;
            flame_now = (key_state & KS_THRUST) && (flame_flick & 1);
            ship_render(flame_now);
            flame_was_drawn = flame_now;
            ship_was_drawn = 1;
        } else {
            ship_was_drawn = 0;
            flame_was_drawn = 0;
        }

        bullets_commit();
        ufo_bullet_commit();
        debris_render();
        asteroid_debris_render();

        /* ===== POST-RENDER (hors fenêtre flicker) ===== */

        if (lives > lives_prev && sfx_id == FX_NONE) {
            sound_play_fx(FX_LIFE);
        }
        lives_prev = lives;

        if (!gameover) {
            if (thump_timer == 0) {
                static unsigned char thump_toggle = 0;
                unsigned char n = asteroids_count();
                unsigned char period;
                if (n >= 8)      period = THUMP_PERIOD_BASE;
                else if (n >= 4) period = THUMP_PERIOD_BASE - 10;
                else if (n >= 2) period = THUMP_PERIOD_BASE - 20;
                else             period = THUMP_PERIOD_MIN;
                sound_play_fx(thump_toggle ? FX_THUMP_2 : FX_THUMP);
                thump_toggle = !thump_toggle;
                thump_timer = period;
            } else {
                thump_timer--;
            }
        }

        if (gameover && !prev_gameover) {
            final_score = score;
            new_hiscore_pos = hiscores_insert(final_score);
            ufo_kill();
            gameover_elapsed = 0;
            gameover_armed   = 1;
        }
        prev_gameover = gameover;

        if (gameover && gameover_elapsed < 255) gameover_elapsed++;

        if (current_wave != wave_displayed) {
            if (wave_displayed != 0) wave_label_erase(WAVE_HUD_Y, wave_displayed);
            wave_label_draw(WAVE_HUD_Y, current_wave);
            wave_displayed = current_wave;
        }

        hud_draw();

        if (gameover) {
            if (gameover_elapsed >= DEATH_EXPLOSION_END
                && gameover_elapsed < DEATH_HOF_FRAME
                && !gameover_text_drawn) {
                gameover_draw();
                gameover_text_drawn = 1;
            }
            if (gameover_elapsed >= DEATH_HOF_FRAME && gameover_text_drawn) {
                gameover_erase();
                gameover_text_drawn = 0;
            }
            if (gameover_elapsed >= DEATH_HOF_FRAME && !hiscores_drawn) {
                hiscores_draw_table();
                hiscores_drawn = 1;
            }
            if (gameover_elapsed >= DEATH_HOF_FRAME
                && !gameover_armed
                && !prompt_drawn) {
                presspace_draw(GO_PRESS_Y);
                quit_label_draw(GO_QUIT_Y);
                prompt_drawn = 1;
            }
        }

        frame_wait();
    }
}
