/*
 * keys.c — Écran de configuration des touches (Phase 40) — port Neo6502
 *
 * Accessible depuis l'écran titre (touche K). Re-mappe séquentiellement
 * les 5 actions (LEFT, RIGHT, THRUST, FIRE, HYPER) : l'action courante
 * est soulignée, presser une touche connue l'assigne ; ESC annule tout
 * et restaure le mapping d'entrée. Les doublons et les touches sans nom
 * (key_name NULL : modificateurs, ESC réservé…) sont refusés.
 *
 * Le mapping (key_map, codes HID) vit en RAM — pas de persistance.
 * Cadence : vsync_wait (compteur de trames 60 Hz).
 */

#include "keys.h"
#include "font.h"
#include "line.h"
#include "sound.h"
#include "neo_time.h"
#include "screen.h"

static const char * const act_label[5] = {
    "LEFT", "RIGHT", "THRUST", "FIRE", "HYPER"
};

/* Layout : titre + 5 lignes label/nom, centré sur 320 px. */
#define KTITLE_Y  32
#define KROW0_Y   68
#define KROW_H    22
#define KLBL_X    60
#define KNAME_X   170

static int row_y(unsigned char i)
{
    return KROW0_Y + i * KROW_H;
}

static void underline(unsigned char i)
{
    lx0 = KLBL_X;
    ly0 = row_y(i) + 12;
    lx1 = KLBL_X + 70;
    ly1 = ly0;
    draw_line_xor();
}

/* Dessiner/effacer (XOR idempotent) tout l'écran : titre + labels +
 * noms courants. Valide comme erase uniquement si l'affichage est
 * synchrone de key_map — garanti par le flux de keys_screen. */
static void screen_xor(void)
{
    unsigned char i;
    text_draw("CONTROLS", (SCR_W - 96) / 2, KTITLE_Y);
    for (i = 0; i < 5; i++) {
        text_draw(act_label[i], KLBL_X, row_y(i));
        text_draw(key_name[key_map[i]], KNAME_X, row_y(i));
    }
}

static void wait_release(void)
{
    do { vsync_wait(); } while (key_probe() != KEY_PROBE_NONE);
}

void keys_screen(void)
{
    unsigned char i, j, k;
    unsigned char cancel = 0;
    unsigned char saved[5];

    for (i = 0; i < 5; i++) saved[i] = key_map[i];

    screen_xor();

    for (i = 0; i < 5 && !cancel; i++) {
        underline(i);
        wait_release();         /* ne pas capter la touche précédente */
        for (;;) {
            vsync_wait();
            k = key_probe();
            if (k == KEY_PROBE_NONE) continue;
            if (k == KEY_PROBE_ESC) { cancel = 1; break; }
            if (key_name[k] == 0) continue;      /* touche refusée */
            for (j = 0; j < 5; j++)              /* doublon ? */
                if (j != i && key_map[j] == k) break;
            if (j < 5) continue;
            break;
        }
        if (!cancel && k != key_map[i]) {
            text_draw(key_name[key_map[i]], KNAME_X, row_y(i)); /* erase */
            key_map[i] = k;
            text_draw(key_name[k], KNAME_X, row_y(i));          /* draw */
        }
        if (!cancel) sound_play_fx(FX_FIRE);     /* feedback assignation */
        underline(i);           /* erase souligné */
    }

    wait_release();
    screen_xor();               /* erase écran (synchro key_map ✓) */

    if (cancel) {
        for (i = 0; i < 5; i++) key_map[i] = saved[i];
    }
}
