#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <types.h>

struct Camera {
    Vec3 target;
    int32_t model_offset[3];
    float yOffset;
    float fov;
    float distance;
    int16_t pitch;
    int16_t yaw;
    int16_t roll;
};

extern Camera g_Camera;

#endif
