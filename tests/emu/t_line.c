/* t_line.c — primitives : point, ligne horizontale, verticale, diagonale,
 * carré semi-ouvert, puis carré dessiné 2× (doit disparaître). */
#include "line.h"
#include "neo_time.h"

static void sq(int x, int y, int s, void (*f)(void))
{
    lx0 = x;     ly0 = y;     lx1 = x + s; ly1 = y;     f();
    lx0 = x + s; ly0 = y;     lx1 = x + s; ly1 = y + s; f();
    lx0 = x + s; ly0 = y + s; lx1 = x;     ly1 = y + s; f();
    lx0 = x;     ly0 = y + s; lx1 = x;     ly1 = y;     f();
}

int main(void)
{
    gfx_init();
    lx0 = 10; ly0 = 10; plot_dot();
    lx0 = 20; ly0 = 20; lx1 = 60; ly1 = 20; draw_line_xor();
    lx0 = 100; ly0 = 20; lx1 = 100; ly1 = 80; draw_line_xor();
    lx0 = 120; ly0 = 20; lx1 = 180; ly1 = 80; draw_line_xor();
    lx0 = 300; ly0 = 200; lx1 = 250; ly1 = 100; draw_line_xor();
    sq(20, 120, 30, draw_line_xor);
    sq(80, 120, 30, draw_line_xor_open);
    sq(140, 120, 30, draw_line_xor_open);
    sq(140, 120, 30, draw_line_xor_open);
    for (;;) vsync_wait();
    return 0;
}
