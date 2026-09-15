/*
 * phys.h — Intégration 8.8 et collision torique (port Neo6502)
 *
 * Sur Oric, position = octet entier + octet fractionnaire recomposés en
 * 16 bits. Avec un écran de 320 px, (x << 8) déborde 16 bits : on garde
 * l'entier en int et on propage la retenue de la partie fractionnaire.
 */

#ifndef PHYS_H
#define PHYS_H

/* pos += v (8.8 signé), modulo span (wraparound). */
void integrate(int *pos, unsigned char *frac, int v, int span);

/* Distance L∞ torique ≤ r sur les deux axes (écran SCR_W × SCR_H). */
unsigned char collide(int x1, int y1, int x2, int y2, unsigned char r);

#endif /* PHYS_H */
