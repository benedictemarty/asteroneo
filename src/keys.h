/*
 * keys.h — Clavier : scan des actions, sonde et écran de configuration
 *
 * Neo6502 : l'état de chaque touche est lu par l'API 1,2 (Key Status),
 * indexée par code HID USB. key_map[] contient les 5 codes HID des
 * actions (ordre = bits de key_state) ; ESC (HID $29) est fixe.
 */

#ifndef KEYS_H
#define KEYS_H

/* Bits de key_state (identiques à la version Oric) */
#define KS_LEFT    0x01
#define KS_RIGHT   0x02
#define KS_THRUST  0x04
#define KS_FIRE    0x08
#define KS_HYPER   0x10
#define KS_ESC     0x20

/* Codes HID utiles */
#define HID_K      0x0E
#define HID_ESC    0x29
#define HID_SPACE  0x2C
#define HID_RIGHT  0x4F
#define HID_LEFT   0x50
#define HID_DOWN   0x51
#define HID_UP     0x52
#define HID_MAX    0x64     /* dernier code interrogé par key_probe */

#define KEY_PROBE_NONE 0xFF
#define KEY_PROBE_K    HID_K
#define KEY_PROBE_ESC  HID_ESC

extern unsigned char key_state;
extern unsigned char key_map[5];
extern const char * const key_name[HID_MAX + 1];

/* Lit les 5 actions + ESC → key_state (6 appels API). */
void key_scan(void);

/* Code HID de la première touche pressée, ou KEY_PROBE_NONE. Réservé aux
 * écrans (titre / config) : ~100 appels API. */
unsigned char key_probe(void);

/* Écran CONTROLS : re-mappe les 5 actions ; ESC annule. */
void keys_screen(void);

#endif /* KEYS_H */
