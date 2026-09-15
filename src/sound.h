/*
 * sound.h — Effets sonores sur le générateur du Neo6502 (API groupe 8)
 *
 * Même interface et mêmes identifiants que le driver AY de la version
 * Oric. Architecture 3 canaux conservée :
 *   canal 0 = effets primaires (fire, explode S/M/L, hyper, thrust, life)
 *   canal 1 = thump (Beat1/Beat2)
 *   canal 2 = UFO (large/small), relancé tant que l'UFO est actif
 *   canal 3 = doublure d'octave du jingle titre
 * Chaque effet est une courte file de notes (8,7 : fréquence, durée en
 * centièmes, forme carré/bruit, volume) exécutée par le RP2040 ; le 6502
 * ne garde qu'un compteur de frames par canal (sound_tick).
 */

#ifndef SOUND_H
#define SOUND_H

#define FX_NONE         0
#define FX_FIRE         1
#define FX_EXPLODE      2
#define FX_THUMP        3
#define FX_HYPER        4
#define FX_THRUST       5
#define FX_LIFE         6
#define FX_UFO          7
#define FX_BANG_MEDIUM  8
#define FX_BANG_SMALL   9
#define FX_THUMP_2     10
#define FX_UFO_SMALL   11

extern unsigned char sfx_id;      /* canal 0 : effet en cours (FX_NONE = libre) */
extern unsigned char sfx_timer;   /* canal 0 : frames restantes */

void sound_init(void);
void sound_tick(void);            /* 1 appel par trame (vsync_wait / frame_wait) */
void sound_play_fx(unsigned char fx_id);
void sound_stop_ufo(void);
void sound_mute(void);            /* coupe tout (sortie du jeu) */

/* Jingle titre : index chromatique depuis C3 (C4 = 12, C5 = 24). */
void tune_play_note(unsigned char idx);
void tune_stop(void);

#endif /* SOUND_H */
