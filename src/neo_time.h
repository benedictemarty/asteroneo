/*
 * neo_time.h — Cadence : compteur de trames du Neo6502 (5,37, 60 Hz)
 *
 * Remplace le Timer 1 du VIA + IRQ de la version Oric. Aucune interruption
 * sur le firmware standard : la boucle de jeu interroge le compteur de
 * trames et appelle sound_tick() à chaque trame attendue.
 */

#ifndef NEO_TIME_H
#define NEO_TIME_H

/* Octet bas du compteur de trames (5,37) — modulo 256. */
unsigned char frame_tick(void);

/* Attend la trame suivante (une seule), en faisant avancer le son. */
void vsync_wait(void);

#endif /* NEO_TIME_H */
