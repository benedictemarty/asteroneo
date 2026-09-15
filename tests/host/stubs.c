/* stubs.c — remplaçants host des primitives de tracé (line.h) :
 * enregistrent les segments/pixels XOR dans un tampon 320×240. */
#include <string.h>
#include "screen.h"

int lx0, ly0, lx1, ly1;
unsigned char fb[SCR_H][SCR_W];
unsigned long stub_calls;

void gfx_init(void) { memset(fb, 0, sizeof fb); stub_calls = 0; }
void gfx_clear(void) { memset(fb, 0, sizeof fb); }

static void px(int x, int y)
{
    if (x >= 0 && y >= 0 && x < SCR_W && y < SCR_H) fb[y][x] ^= 1;
}

/* EFLA du firmware (efla.cpp) : [P0, P1[ */
static void efla(int x, int y, int x2, int y2)
{
    int yLonger = 0, inc, end, sl = y2 - y, ll = x2 - x;
    double dec, j = 0.0;
    if (x < 0 || y < 0 || x >= SCR_W || y >= SCR_H) return;
    if (x2 < 0 || y2 < 0 || x2 >= SCR_W || y2 >= SCR_H) return;
    if ((sl < 0 ? -sl : sl) > (ll < 0 ? -ll : ll)) { int t = sl; sl = ll; ll = t; yLonger = 1; }
    end = ll;
    if (ll < 0) { inc = -1; ll = -ll; } else inc = 1;
    dec = (ll == 0) ? (double)sl : ((double)sl / (double)ll);
    for (int i = 0; i != end; i += inc) {
        if (yLonger) px(x + (int)j, y + i); else px(x + i, y + (int)j);
        j += dec;
    }
}

void plot_dot(void) { stub_calls++; px(lx0, ly0); }
void draw_line_xor(void) { stub_calls += 2; efla(lx0, ly0, lx1, ly1); px(lx1, ly1); }
void draw_line_xor_open(void) { stub_calls++; efla(lx1, ly1, lx0, ly0); }

unsigned long fb_count(void)
{
    unsigned long n = 0;
    for (int y = 0; y < SCR_H; y++) for (int x = 0; x < SCR_W; x++) n += fb[y][x];
    return n;
}

const signed char *poly_vx, *poly_vy;
unsigned char poly_n;
int poly_cx, poly_cy;

/* Même sémantique que _poly_xor (neo_gfx.s) : segments ]Pi-1, Pi], clip par segment. */
void poly_xor(void)
{
    unsigned char i, n = poly_n;
    int px, py, qx, qy;
    if (n == 0) return;
    px = poly_cx + poly_vx[n - 1]; py = poly_cy + poly_vy[n - 1];
    for (i = 0; i < n; i++) {
        qx = poly_cx + poly_vx[i]; qy = poly_cy + poly_vy[i];
        if (px >= 0 && px < SCR_W && py >= 0 && py < SCR_H &&
            qx >= 0 && qx < SCR_W && qy >= 0 && qy < SCR_H) {
            stub_calls++;
            efla(qx, qy, px, py);
        }
        px = qx; py = qy;
    }
}
