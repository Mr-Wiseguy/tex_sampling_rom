#ifndef __COLLISION_H__
#define __COLLISION_H__

#include <types.h>

#define IS_NOT_LEAF_NODE(bvhNode) ((bvhNode).triCount != 0)
#define IS_LEAF_NODE(bvhNode) ((bvhNode).triCount == 0)

struct AABB {
    Vec3 min;
    Vec3 max;
};

uint32_t testVerticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax);
float verticalRayVsAABB(Vec3 rayStart, float lengthInv, AABB *box, float tmin, float tmax);
uint32_t testRayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax);
float rayVsAABB(Vec3 rayStart, Vec3 rayDirInv, AABB *box, float tmin, float tmax);

float raycastVertical(Vec3 rayOrigin, float rayLength, float tmin, float tmax, SurfaceType *floorSurface);
float raycast(Vec3 rayOrigin, Vec3 rayDir, float tmin, float tmax, SurfaceType *floorSurface);

void resolve_circle_collision(Vec3 pos, Vec3 vel, Vec3 hit_pos, float hit_dist, float radius);
int circle_aabb_intersect(float x, float z, float min_x, float max_x, float min_z, float max_z, float rad_sq, float* dist_out, Vec3 hit_out);

#endif
