#include <cfloat>

#include <gfx.h>
#include <mem.h>
#include <ecs.h>
#include <collision.h>
#include <surface_types.h>

float raycastVertical(UNUSED Vec3 rayOrigin, UNUSED float rayLength, UNUSED float tmin, UNUSED float tmax, UNUSED SurfaceType *floorSurface)
{
    // TODO Search the grid

    *floorSurface = -1;
    return FLT_MAX;
}

float raycast(UNUSED Vec3 rayOrigin, UNUSED Vec3 rayDir, UNUSED float tmin, UNUSED float tmax, UNUSED SurfaceType *floorSurface)
{
    // TODO search the grid

    *floorSurface = -1;
    return FLT_MAX;
}
