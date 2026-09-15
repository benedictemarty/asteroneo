/* test_keys.c — table des noms HID de neo_input.c : taille, entrées clés,
 * touches refusées, noms compatibles avec la police (A-Z, 0-9, espace). */
#include <stdio.h>
#define NEO_H                                  /* neutralise l'accès à $FF00 */
#define NEO_CMD dummy
#define NEO_FN  dummy
#define NEO_ERR dummy
#define NEO_P   dummyp
#define neo_wait() ((void)0)
#define neo_call(g, f) ((void)0)
#define NEO_G_SYSTEM 1
#define NEO_F_KEY_STATUS 2
static volatile unsigned char dummy;
static volatile unsigned char dummyp[16];
#include "../../src/neo_input.c"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    CHECK(sizeof(key_name) / sizeof(key_name[0]) == HID_MAX + 1);
    CHECK(key_name[0x04][0] == 'A' && key_name[0x1D][0] == 'Z');
    CHECK(key_name[0x1E][0] == '1' && key_name[0x27][0] == '0');
    CHECK(key_name[HID_SPACE] && key_name[HID_SPACE][0] == 'S');
    CHECK(key_name[HID_LEFT] && key_name[HID_LEFT][0] == 'L');
    CHECK(key_name[HID_RIGHT] && key_name[HID_RIGHT][0] == 'R');
    CHECK(key_name[HID_UP] && key_name[HID_UP][0] == 'U');
    CHECK(key_name[HID_DOWN] && key_name[HID_DOWN][0] == 'D');
    CHECK(key_name[HID_ESC] == 0);
    CHECK(key_name[HID_K] && key_name[HID_K][0] == 'K');
    CHECK(key_name[0x28] && key_name[0x28][0] == 'R');   /* RETURN */
    CHECK(key_name[0x63] && key_name[0x63][0] == 'K');   /* KPDOT */
    for (int k = 0; k <= HID_MAX; k++) {
        if (!key_name[k]) continue;
        for (const char *s = key_name[k]; *s; s++)
            CHECK((*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == ' ');
    }
    CHECK(key_map[0] == HID_LEFT && key_map[3] == HID_SPACE && key_map[4] == HID_DOWN);
    printf(fails ? "test_keys : %d échec(s)\n" : "test_keys : OK\n", fails);
    return fails ? 1 : 0;
}
