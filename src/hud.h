/*
 * hud.h — Score + vies, rendu 7-segments + mini-triangles (Phase 5) — Neo6502
 */

#ifndef HUD_H
#define HUD_H

#define HUD_LIVES_INIT  3
#define HUD_EXTRA_BONUS 10000U

extern unsigned int  score;
extern unsigned int  score_extra;
extern unsigned char lives;
extern unsigned char gameover;

void hud_init(void);
void hud_erase(void);
void hud_draw(void);
void hud_add_score(unsigned int delta);
void hud_lose_life(void);
void hud_xor_5digits(unsigned int s, int px, int py);
void hud_xor_digit(unsigned char d, int px, int py);

#endif /* HUD_H */
