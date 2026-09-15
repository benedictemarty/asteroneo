/* test_phys.c — intégration 8.8 avec retenue et wraparound, collision torique */
#include <stdio.h>
#include "phys.h"
#include "screen.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    int p; unsigned char f;

    /* +1.5 px par pas */
    p = 10; f = 0; integrate(&p, &f, 0x0180, SCR_W); CHECK(p == 11 && f == 0x80);
    integrate(&p, &f, 0x0180, SCR_W); CHECK(p == 13 && f == 0x00);
    /* -1/256 : retenue négative */
    p = 10; f = 0; integrate(&p, &f, -1, SCR_W); CHECK(p == 9 && f == 0xFF);
    integrate(&p, &f, 1, SCR_W); CHECK(p == 10 && f == 0);
    /* -0.5 px depuis 0 → wrap à SCR_W-1 */
    p = 0; f = 0; integrate(&p, &f, -0x80, SCR_W); CHECK(p == SCR_W - 1 && f == 0x80);
    /* +8 px depuis 315 → wrap */
    p = 315; f = 0; integrate(&p, &f, 0x0800, SCR_W); CHECK(p == 3 && f == 0);
    /* axe Y : 239 + 1 → 0 */
    p = 239; f = 0; integrate(&p, &f, 0x0100, SCR_H); CHECK(p == 0);
    /* sous-pixel cumulé : 256 pas de +1/256 = +1 px */
    p = 100; f = 0; for (int i = 0; i < 256; i++) integrate(&p, &f, 1, SCR_W);
    CHECK(p == 101 && f == 0);

    /* collisions */
    CHECK(collide(100, 100, 105, 103, 5) == 1);
    CHECK(collide(100, 100, 106, 100, 5) == 0);
    CHECK(collide(2, 100, 318, 100, 5) == 1);      /* à travers le bord X */
    CHECK(collide(100, 2, 100, 237, 5) == 1);      /* à travers le bord Y */
    CHECK(collide(2, 100, 310, 100, 5) == 0);
    CHECK(collide(0, 0, 319, 239, 1) == 1);        /* coin */

    printf(fails ? "test_phys : %d échec(s)\n" : "test_phys : OK\n", fails);
    return fails ? 1 : 0;
}
