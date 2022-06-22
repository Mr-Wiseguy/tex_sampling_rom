#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include <types.h>

#define GRAVITY (-16.67f)

// Max step up and down heights for objects
#define MAX_STEP_UP   100.0f
#define MAX_STEP_DOWN 20.0f

typedef struct GravityParams_t {
    float accel;
    float terminalVelocity;
} GravityParams;

void physicsTick();

#endif
