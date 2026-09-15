/*
 * line.h — Primitives de tracé XOR (src/asm/neo_gfx.s)
 *
 * Même interface que sur Oric (lx0/ly0 → lx1/ly1 en zero page), mais en
 * int 16 bits et servie par l'API graphique du Neo6502 :
 *   - gfx_init : mode XOR (5,1 And=$FF Xor=INK_COLOUR), écran effacé ;
 *   - draw_line_xor : ligne fermée (5,2) ;
 *   - draw_line_xor_open : tous les pixels sauf le point de départ
 *     (ligne 5,2 puis re-XOR du départ 5,5) — polygones à sommets partagés ;
 *   - plot_dot : un pixel (5,5).
 * Idempotence : deux appels identiques = effacement.
 * Contrat : coordonnées dans [0, SCR_W-1] × [0, SCR_H-1] (clip appelant).
 */

#ifndef LINE_H
#define LINE_H

extern int lx0, ly0, lx1, ly1;
#pragma zpsym ("lx0")
#pragma zpsym ("ly0")
#pragma zpsym ("lx1")
#pragma zpsym ("ly1")

void gfx_init(void);
void gfx_clear(void);
void draw_line_xor(void);
void draw_line_xor_open(void);
void plot_dot(void);

#endif /* LINE_H */
