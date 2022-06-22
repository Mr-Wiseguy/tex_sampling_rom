#ifndef __MODEL_H__
#define __MODEL_H__

#include <platform_gfx.h>
#include <gfx.h>

#include <types.h>

///////////////////////
// Animation defines //
///////////////////////

// Animation flags
#define ANIM_LOOP (1 << 0)
#define ANIM_HAS_TRIGGERS (1 << 1)

// Joint table flags
#define CHANNEL_POS_X (1 << 0)
#define CHANNEL_POS_Y (1 << 1)
#define CHANNEL_POS_Z (1 << 2)
#define CHANNEL_ROT_X (1 << 3)
#define CHANNEL_ROT_Y (1 << 4)
#define CHANNEL_ROT_Z (1 << 5)
#define CHANNEL_SCALE_X (1 << 6)
#define CHANNEL_SCALE_Y (1 << 7)
#define CHANNEL_SCALE_Z (1 << 8)

#define ANIM_COUNTER_FACTOR 16.0f
#define ANIM_COUNTER_SHIFT 4
#define ANIM_COUNTER_TO_FRAME(x) ((x) >> (ANIM_COUNTER_SHIFT))

struct AnimState {
    Animation* anim;
    uint16_t counter; // Frame counter of format 12.4
    int8_t speed; // Animation playback speed of format s3.4
    int8_t triggerIndex; // Index of the previous trigger
};

#endif
