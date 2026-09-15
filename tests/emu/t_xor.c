/* t_xor.c — test d'idempotence XOR : 3 astéroïdes (ids 8, 9, 10) tracés
 * une fois à gauche, deux fois (= effacés) à droite. Attendu : rien à
 * droite. Puis boucle infinie. */
#include "asteroids.h"
#include "line.h"
#include "neo_time.h"


int main(void)
{
    unsigned char i;
    gfx_init();
    for (i = 0; i < 3; i++) {
        asteroids[0].size = 2; asteroids[0].shape = i; asteroids[0].active = 1;
        asteroids[0].x = 40 + i * 60; asteroids[0].y = 60;
        asteroids[0].prev_x = asteroids[0].x; asteroids[0].prev_y = 60;
        asteroids[0].drawn = 0;
        asteroids_render();                 /* draw à (x,y) */
        asteroids[0].x = 40 + i * 60; asteroids[0].y = 160;
        asteroids[0].prev_x = asteroids[0].x; asteroids[0].prev_y = 160;
        asteroids[0].drawn = 0;
        asteroids_render();                 /* draw en bas */
        asteroids_render();                 /* erase (prev) + draw (x,y) = net 1 */
        asteroids[0].active = 0;
        asteroids_render();                 /* erase → rien en bas */
    }
    for (;;) vsync_wait();
    return 0;
}
