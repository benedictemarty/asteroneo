/*
 * neo.h — Accès à l'API du Neo6502 (bloc de contrôle $FF00)
 *
 * Protocole : écrire les paramètres ($FF04..), la fonction ($FF01), puis
 * le groupe ($FF00) ; le RP2040 exécute et remet $FF00 à 0. Une erreur
 * est signalée dans $FF02. Toute écriture de paramètres doit attendre
 * que la commande précédente soit terminée (neo_wait).
 */

#ifndef NEO_H
#define NEO_H

#define NEO_CMD   (*(volatile unsigned char *)0xFF00)
#define NEO_FN    (*(volatile unsigned char *)0xFF01)
#define NEO_ERR   (*(volatile unsigned char *)0xFF02)
#define NEO_P     ((volatile unsigned char *)0xFF04)   /* P0..P7 ; P8 = $FF0C */

#define neo_wait()        do { while (NEO_CMD) ; } while (0)
#define neo_call(g, f)    do { NEO_FN = (f); NEO_CMD = (g); neo_wait(); } while (0)

/* Groupes */
#define NEO_G_SYSTEM    1
#define NEO_G_CONSOLE   2
#define NEO_G_GRAPHICS  5
#define NEO_G_INPUT     7
#define NEO_G_SOUND     8

/* Système */
#define NEO_F_TIMER       1     /* P0..3 = timer 100 Hz */
#define NEO_F_KEY_STATUS  2     /* P0 = code HID → P0 = état, P1 = modificateurs */

/* Console */
#define NEO_F_CLEAR_SCREEN 12
#define NEO_F_CURSOR_OFF   0    /* pas de fonction dédiée : on efface + n'écrit rien */

/* Graphique */
#define NEO_F_GFX_DEFAULTS 1    /* P0 and, P1 xor, P2 solid, P3 taille, P4 flip */
#define NEO_F_DRAW_LINE    2
#define NEO_F_DRAW_RECT    3
#define NEO_F_DRAW_PIXEL   5
#define NEO_F_FRAME_COUNT  37   /* P0..3 = trames depuis l'allumage (60 Hz) */

/* Son */
#define NEO_F_SND_RESET      1
#define NEO_F_SND_RESET_CH   2
#define NEO_F_SND_QUEUE_EXT  7  /* P0 canal, P1-2 Hz, P3-4 durée cs, P5-6 slide, P7 type, P8 volume */
#define NEO_F_SND_STATUS     6

#define NEO_SND_SQUARE 0
#define NEO_SND_NOISE  1

#endif /* NEO_H */
