#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <types.h>

void playSound(uint32_t soundIndex);
void stopMusic();
void playMusic(uint32_t song_idx);

enum Sfx {
    metal_hit,
    zap,
    bootup,
    song0,
    card_get,
    charge,
    beam,
    song_title,
    song1,
    song2,
    hijack,
    small_boom,
    clank,
    crash,
    door,
    boom,
    launch,
    grease,
};

#endif
