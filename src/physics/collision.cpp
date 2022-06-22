#include <cfloat>

#include <mathutils.h>
#include <collision.h>
#include <physics.h>
#include <types.h>

extern "C" {
#include <debug.h>
}

uint32_t testVerticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    if (rayStart[0] > box->max[0] || rayStart[0] < box->min[0] || rayStart[2] > box->max[2] || rayStart[2] < box->min[2])
        return 0;

    // y
    t0 = (box->min[1] - rayStart[1]) * lengthInv;
    t1 = (box->max[1] - rayStart[1]) * lengthInv;

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;
    
    return 1;
}

float verticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    if (rayStart[0] > box->max[0] || rayStart[0] < box->min[0] || rayStart[2] > box->max[2] || rayStart[2] < box->min[2])
        return FLT_MAX;

    // y
    t0 = (box->min[1] - rayStart[1]) * lengthInv;
    t1 = (box->max[1] - rayStart[1]) * lengthInv;

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;
    
    return tmin;
}

uint32_t testRayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    // x
    t0 = (box->min[0] - rayStart[0]) * rayDirInv[0];
    t1 = (box->max[0] - rayStart[0]) * rayDirInv[0];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;

    // y
    t0 = (box->min[1] - rayStart[1]) * rayDirInv[1];
    t1 = (box->max[1] - rayStart[1]) * rayDirInv[1];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;

    // z
    t0 = (box->min[2] - rayStart[2]) * rayDirInv[2];
    t1 = (box->max[2] - rayStart[2]) * rayDirInv[2];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return 0;
    
    return 1;
}

float rayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax)
{
    float t0, t1;

    // x
    t0 = (box->min[0] - rayStart[0]) * rayDirInv[0];
    t1 = (box->max[0] - rayStart[0]) * rayDirInv[0];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;

    // y
    t0 = (box->min[1] - rayStart[1]) * rayDirInv[1];
    t1 = (box->max[1] - rayStart[1]) * rayDirInv[1];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;

    // z
    t0 = (box->min[2] - rayStart[2]) * rayDirInv[2];
    t1 = (box->max[2] - rayStart[2]) * rayDirInv[2];

    tmin = MAX(tmin, MIN(t0, t1));
    tmax = MIN(tmax, MAX(t0, t1));
    
    if (tmin > tmax)
        return FLT_MAX;
    
    return tmin;
}

#define VEL_THRESHOLD 0.1f

void resolve_circle_collision(Vec3 pos, Vec3 vel, Vec3 hit_pos, float hit_dist, float radius)
{
    float dx = hit_pos[0] - pos[0];
    float dz = hit_pos[2] - pos[2];
    if (hit_dist > EPSILON)
    {
        float nx = dx * (1.0f / hit_dist);
        float nz = dz * (1.0f / hit_dist);

        float vel_dot_norm = nx * vel[0] + nz * vel[2];
        vel[0] -= vel_dot_norm * nx;
        vel[2] -= vel_dot_norm * nz;

        float push_dist = radius - hit_dist;
        pos[0] -= push_dist * nx;
        pos[2] -= push_dist * nz;
    }
    else
    {
        vel[0] = 0;
        vel[2] = 0;
    }
}
