/*
 * neo_input.c — Lecture clavier par l'API 1,2 (Key Status, code HID)
 *
 * Remplace input.s (scan matrice VIA/PSG de l'Oric). key_map[] est le
 * mapping courant des 5 actions (défaut : flèches + SPACE), modifiable
 * par l'écran CONTROLS (keys.c).
 */

#include "neo.h"
#include "keys.h"

unsigned char key_state;
unsigned char key_map[5] = { HID_LEFT, HID_RIGHT, HID_UP, HID_SPACE, HID_DOWN };

static unsigned char key_down(unsigned char hid)
{
    neo_wait();
    NEO_P[0] = hid;
    neo_call(NEO_G_SYSTEM, NEO_F_KEY_STATUS);
    return NEO_P[0];
}

void key_scan(void)
{
    unsigned char s = 0, i, m;
    for (i = 0, m = 1; i < 5; i++, m <<= 1) {
        if (key_down(key_map[i])) s |= m;
    }
    if (key_down(HID_ESC)) s |= KS_ESC;
    key_state = s;
}

unsigned char key_probe(void)
{
    unsigned char k;
    for (k = 4; k <= HID_MAX; k++) {
        if (key_down(k)) return k;
    }
    return KEY_PROBE_NONE;
}

/* Noms affichables (police A-Z 0-9 uniquement) ; 0 = touche refusée par
 * l'écran de configuration (inconnue, modificateur ou ESC réservé). */
const char * const key_name[HID_MAX + 1] = {
    /* 00 */ 0, 0, 0, 0,
    /* 04 */ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    /* 11 */ "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    /* 1E */ "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
    /* 28 */ "RETURN", 0 /* ESC */, "BACKSPACE", "TAB", "SPACE",
    /* 2D */ "MINUS", "EQUAL", "LBRK", "RBRK", "BSLASH", 0,
    /* 33 */ "SEMI", "QUOTE", "GRAVE", "COMMA", "DOT", "SLASH", 0 /* caps */,
    /* 3A */ "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
    /* 46 */ 0, 0, 0, "INS", "HOME", "PGUP", "DEL", "END", "PGDN",
    /* 4F */ "RIGHT", "LEFT", "DOWN", "UP",
    /* 53 */ 0, "KPDIV", "KPMUL", "KPMINUS", "KPPLUS", "KPENTER",
    /* 59 */ "KP1", "KP2", "KP3", "KP4", "KP5", "KP6", "KP7", "KP8", "KP9", "KP0",
    /* 63 */ "KPDOT", 0
};
