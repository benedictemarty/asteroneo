/*
 * phys.c — intégration 8.8 et collision torique
 */

#include "phys.h"
#include "screen.h"

void integrate(int *pos, unsigned char *frac, int v, int span)
{
    unsigned int f = (unsigned int)*frac + (unsigned char)v;   /* octet bas de v */
    int p = *pos + (v >> 8) + (int)(f >> 8);                    /* >> arithmétique */
    *frac = (unsigned char)f;
    if (p < 0) p += span;
    else if (p >= span) p -= span;
    *pos = p;
}

unsigned char collide(int x1, int y1, int x2, int y2, unsigned char r)
{
    /* Distance torique min(d, SPAN-d) par axe : une entité proche d'un
     * bord est aussi dessinée en instance fantôme de l'autre côté. */
    int d;
    d = x1 - x2; if (d < 0) d = -d;
    if (d > SCR_HALF_W) d = SCR_W - d;
    if (d > r) return 0;
    d = y1 - y2; if (d < 0) d = -d;
    if (d > SCR_HALF_H) d = SCR_H - d;
    if (d > r) return 0;
    return 1;
}
