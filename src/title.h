/*
 * title.h — Écran titre Phase 9c
 */

#ifndef TITLE_H
#define TITLE_H

void title_draw(void);
void title_erase(void);
void gameover_draw(void);
void gameover_erase(void);
void presspace_draw(int py);
void presspace_erase(int py);
/* Phase 10d — affichage "WAVE n" centré horizontal */
void wave_label_draw(int py, unsigned char digit);
void wave_label_erase(int py, unsigned char digit);
/* Phase 15 — affichage "HIGH SCORES" centré horizontal */
void hiscores_label_draw(int py);
void hiscores_label_erase(int py);

/* Phase 18i — "OR ESC TO STOP" sous PRESS SPACE en game over */
void quit_label_draw(int py);
void quit_label_erase(int py);

/* Phase 40 — "K CONTROLS" sur l'écran titre */
void keyshint_draw(int py);
void keyshint_erase(int py);

#endif /* TITLE_H */
