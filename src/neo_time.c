/*
 * neo_time.c — compteur de trames (API 5,37)
 */

#include "neo.h"
#include "neo_time.h"
#include "sound.h"

unsigned char frame_tick(void)
{
    neo_wait();
    neo_call(NEO_G_GRAPHICS, NEO_F_FRAME_COUNT);
    return NEO_P[0];
}

void vsync_wait(void)
{
    unsigned char t = frame_tick();
    while (frame_tick() == t) ;
    sound_tick();
}
