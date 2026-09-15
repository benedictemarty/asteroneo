/*
 * screen.h — Géométrie de l'écran Neo6502 (mode 0 : 320 × 240)
 *
 * Toutes les coordonnées du jeu sont des int (16 bits signés) : l'axe X
 * dépasse 255. Le terrain occupe l'écran entier ; le wraparound se fait
 * modulo SCR_W / SCR_H (duplication d'instance aux bords, comme sur Oric).
 */

#ifndef SCREEN_H
#define SCREEN_H

#define SCR_W       320
#define SCR_H       240
#define SCR_XMAX    (SCR_W - 1)
#define SCR_YMAX    (SCR_H - 1)
#define SCR_HALF_W  (SCR_W / 2)
#define SCR_HALF_H  (SCR_H / 2)

/* Couleur du tracé XOR (palette par défaut : 255 = blanc). Un fond noir
 * (index 0) XOR 255 = blanc ; le second XOR rend le noir. */
#define INK_COLOUR  255

#endif /* SCREEN_H */
