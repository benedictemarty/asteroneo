/*
 * neo_sound.c — Effets sonores Astéroric sur l'API son du Neo6502
 *
 * Remplace sound.s (AY-3-8912 via VIA). Le générateur du firmware offre
 * 4 canaux, ondes carrée ou bruit, avec file de notes par canal ; les
 * enveloppes de l'AY sont approchées par des notes successives de volume
 * décroissant. Durées en centièmes de seconde (arcade : fire 0,3 s,
 * explosion ≈ 1 s, thump 0,07 s).
 */

#include "neo.h"
#include "sound.h"

#define CH_FX     0
#define CH_THUMP  1
#define CH_UFO    2
#define CH_TUNE2  3

unsigned char sfx_id;
unsigned char sfx_timer;

static unsigned char ufo_id;        /* FX_UFO / FX_UFO_SMALL, ou FX_NONE */
static unsigned char ufo_timer;     /* frames avant relance de la boucle UFO */

/* Ajoute une note à la file du canal (8,7 Queue Sound Extended). */
static void note(unsigned char ch, unsigned int hz, unsigned int cs,
                 unsigned char type, unsigned char vol)
{
    neo_wait();
    NEO_P[0] = ch;
    NEO_P[1] = (unsigned char)hz;
    NEO_P[2] = (unsigned char)(hz >> 8);
    NEO_P[3] = (unsigned char)cs;
    NEO_P[4] = (unsigned char)(cs >> 8);
    NEO_P[5] = 0;                          /* pas de glissando */
    NEO_P[6] = 0;
    NEO_P[7] = type;
    NEO_P[8] = vol;
    neo_call(NEO_G_SOUND, NEO_F_SND_QUEUE_EXT);
}

static void reset_ch(unsigned char ch)
{
    neo_wait();
    NEO_P[0] = ch;
    neo_call(NEO_G_SOUND, NEO_F_SND_RESET_CH);
}

void sound_init(void)
{
    neo_wait();
    neo_call(NEO_G_SOUND, NEO_F_SND_RESET);
    sfx_id = FX_NONE;
    sfx_timer = 0;
    ufo_id = FX_NONE;
    ufo_timer = 0;
}

void sound_mute(void)
{
    sound_init();
}

/* Boucle UFO : 20 cs de notes, relancée par sound_tick tant que ufo_id. */
static void ufo_queue(void)
{
    reset_ch(CH_UFO);
    if (ufo_id == FX_UFO) {
        note(CH_UFO, 1000, 5, NEO_SND_SQUARE, 60);
        note(CH_UFO,  800, 5, NEO_SND_SQUARE, 60);
        note(CH_UFO, 1000, 5, NEO_SND_SQUARE, 60);
        note(CH_UFO,  800, 5, NEO_SND_SQUARE, 60);
    } else {
        note(CH_UFO,  700, 5, NEO_SND_SQUARE, 60);
        note(CH_UFO, 1000, 5, NEO_SND_SQUARE, 60);
        note(CH_UFO, 1100, 5, NEO_SND_SQUARE, 60);
        note(CH_UFO, 1300, 5, NEO_SND_SQUARE, 60);
    }
    ufo_timer = 6;      /* 20 cs à 30 Hz */
}

void sound_play_fx(unsigned char fx)
{
    switch (fx) {
    case FX_THUMP:
        reset_ch(CH_THUMP);
        note(CH_THUMP, 134, 4, NEO_SND_SQUARE, 100);
        note(CH_THUMP,  81, 4, NEO_SND_SQUARE, 80);
        return;
    case FX_THUMP_2:
        reset_ch(CH_THUMP);
        note(CH_THUMP, 129, 4, NEO_SND_SQUARE, 100);
        note(CH_THUMP,  77, 4, NEO_SND_SQUARE, 80);
        return;
    case FX_UFO:
    case FX_UFO_SMALL:
        ufo_id = fx;
        ufo_queue();
        return;
    default:
        break;
    }

    /* Canal 0 : l'effet demandé remplace l'effet en cours. */
    reset_ch(CH_FX);
    sfx_id = fx;
    switch (fx) {
    case FX_FIRE:                       /* bruit 740 Hz, decay ~0,3 s */
        note(CH_FX, 740, 8, NEO_SND_NOISE, 100);
        note(CH_FX, 740, 8, NEO_SND_NOISE, 60);
        note(CH_FX, 740, 8, NEO_SND_NOISE, 30);
        sfx_timer = 8;
        break;
    case FX_EXPLODE:                    /* impact aigu puis corps grave ~1 s */
        note(CH_FX, 1000, 5, NEO_SND_NOISE, 100);
        note(CH_FX,  167, 30, NEO_SND_NOISE, 100);
        note(CH_FX,  167, 30, NEO_SND_NOISE, 60);
        note(CH_FX,  167, 30, NEO_SND_NOISE, 30);
        sfx_timer = 30;
        break;
    case FX_BANG_MEDIUM:
        note(CH_FX, 1000, 5, NEO_SND_NOISE, 100);
        note(CH_FX,  246, 25, NEO_SND_NOISE, 100);
        note(CH_FX,  246, 25, NEO_SND_NOISE, 60);
        note(CH_FX,  246, 25, NEO_SND_NOISE, 30);
        sfx_timer = 25;
        break;
    case FX_BANG_SMALL:
        note(CH_FX, 1000, 5, NEO_SND_NOISE, 100);
        note(CH_FX,  306, 20, NEO_SND_NOISE, 100);
        note(CH_FX,  306, 20, NEO_SND_NOISE, 60);
        note(CH_FX,  306, 20, NEO_SND_NOISE, 30);
        sfx_timer = 20;
        break;
    case FX_HYPER:                      /* ton + bruit ~0,56 s */
        note(CH_FX, 200, 14, NEO_SND_SQUARE, 100);
        note(CH_FX, 300, 14, NEO_SND_NOISE, 80);
        note(CH_FX, 400, 14, NEO_SND_SQUARE, 60);
        note(CH_FX, 600, 14, NEO_SND_NOISE, 30);
        sfx_timer = 17;
        break;
    case FX_THRUST:                     /* rumble grave 82 Hz, relancé */
        note(CH_FX, 82, 24, NEO_SND_SQUARE, 70);
        sfx_timer = 7;
        break;
    case FX_LIFE:                       /* chime extra ship, 4 notes descendantes */
        note(CH_FX, 2841, 8, NEO_SND_SQUARE, 100);
        note(CH_FX, 1389, 8, NEO_SND_SQUARE, 90);
        note(CH_FX,  694, 8, NEO_SND_SQUARE, 70);
        note(CH_FX,  349, 8, NEO_SND_SQUARE, 50);
        sfx_timer = 10;
        break;
    default:
        sfx_id = FX_NONE;
        sfx_timer = 0;
        break;
    }
}

void sound_stop_ufo(void)
{
    ufo_id = FX_NONE;
    ufo_timer = 0;
    reset_ch(CH_UFO);
}

void sound_tick(void)
{
    if (sfx_timer) {
        if (--sfx_timer == 0) sfx_id = FX_NONE;
    }
    if (ufo_id != FX_NONE) {
        if (ufo_timer) ufo_timer--;
        if (ufo_timer == 0) ufo_queue();
    }
}

/* Gamme chromatique C3..C5 (Hz, arrondis) */
static const unsigned int note_hz[25] = {
    131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
    523
};

void tune_play_note(unsigned char idx)
{
    unsigned int hz;
    if (idx > 24) idx = 24;
    hz = note_hz[idx];
    reset_ch(CH_FX);
    reset_ch(CH_TUNE2);
    note(CH_FX, hz, 100, NEO_SND_SQUARE, 100);        /* tenue, coupée par tune_stop */
    note(CH_TUNE2, hz >> 1, 100, NEO_SND_SQUARE, 60); /* octave inférieure */
    sfx_id = FX_NONE;
    sfx_timer = 0;
}

void tune_stop(void)
{
    reset_ch(CH_FX);
    reset_ch(CH_TUNE2);
}
