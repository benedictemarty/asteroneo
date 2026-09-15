/*
 * title.c — Écran titre "ASTERONEO" Phase 9c — port Neo6502 : labels
 * centrés sur SCR_W (320), coordonnées int
 *
 * Phase 40 : les glyphes vectoriels et draw_letter ont migré dans
 * font.c (police complète A-Z pour l'écran de config des touches) ;
 * ce fichier ne garde que les labels du jeu, exprimés en chaînes via
 * text_draw (pitch 12, positions inchangées — snapshot titre vérifié
 * bit-à-bit identique au refactor).
 */

#include "font.h"
#include "hud.h"
#include "screen.h"

/* Dessine "ASTERONEO" centré horizontalement.
 * Largeur totale = 9 * 12 - 4 = 104 pixels → x = (SCR_W - 104) / 2. */
void title_draw(void)
{
    text_draw("ASTERONEO", (SCR_W - 104) / 2, 96);
}

/* Erase = même routine (XOR idempotent) */
void title_erase(void)
{
    title_draw();
}

/* Dessine "GAME OVER" centré en y=70.
 * 9 caractères (avec espace) × 12 = 108 → x = (SCR_W - 108) / 2. */
void gameover_draw(void)
{
    text_draw("GAME OVER", (SCR_W - 108) / 2, 84);
}

void gameover_erase(void)
{
    gameover_draw();
}

/* Dessine "PRESS SPACE" à y donné.
 * 11 caractères × 12 = 132 → x = (SCR_W - 132) / 2. */
void presspace_draw(int py)
{
    text_draw("PRESS SPACE", (SCR_W - 132) / 2, py);
}

void presspace_erase(int py)
{
    presspace_draw(py);
}

/* Phase 10d/10j — affichage "WAVE nn" en haut-centre.
 * 5 caractères ("WAVE ") + 1 ou 2 chiffres.
 * Phase 10j : si wave > 9, afficher 2 chiffres (10, 11).
 * Les chiffres gardent leur placement historique (x+56, pitch 6),
 * distinct du pitch 12 de text_draw. */
void wave_label_draw(int py, unsigned char digit)
{
    int x = SCR_HALF_W - 40;
    text_draw("WAVE", x, py);
    if (digit > 99) digit = 99;
    if (digit < 10) {
        hud_xor_digit(digit, x + 56, py);
    } else {
        hud_xor_digit(digit / 10, x + 56, py);
        hud_xor_digit(digit % 10, x + 62, py);
    }
}

void wave_label_erase(int py, unsigned char digit)
{
    wave_label_draw(py, digit);
}

/* Phase 15 — affichage "HIGH SCORES" centré horizontal.
 * 11 caractères × 12 = 132 px, x = (SCR_W - 132) / 2. */
void hiscores_label_draw(int py)
{
    text_draw("HIGH SCORES", (SCR_W - 132) / 2, py);
}

void hiscores_label_erase(int py)
{
    hiscores_label_draw(py);
}

/* Phase 18i — "OR ESC TO STOP" sous "PRESS SPACE" en game over.
 * 14 caractères (avec 3 espaces) × 12 = 168, x = (SCR_W - 168) / 2. */
void quit_label_draw(int py)
{
    text_draw("OR ESC TO STOP", (SCR_W - 168) / 2, py);
}

void quit_label_erase(int py)
{
    quit_label_draw(py);
}

/* Phase 40 — "K CONTROLS" sur l'écran titre (accès config touches).
 * 10 caractères × 12 = 120, x = (SCR_W - 120) / 2. */
void keyshint_draw(int py)
{
    text_draw("K CONTROLS", (SCR_W - 120) / 2, py);
}

void keyshint_erase(int py)
{
    keyshint_draw(py);
}
