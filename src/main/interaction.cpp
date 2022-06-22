#include <mathutils.h>
#include <interaction.h>

#include <cmath>
#include <cfloat>

#include <ecs.h>

typedef struct FindClosestData_t
{
    float *nearPos;
    float maxDistSq;
    float closestDistSq;
    Vec3 *closestPos;
    Entity *entity;

} FindClosestData;

void debug_printf(const char*, ...);

void findClosestCallback(size_t count, void *arg, UNUSED int numComponents, archetype_t archetype, void **componentArrays, UNUSED size_t *componentSizes)
{
    FindClosestData *findData = (FindClosestData *)arg;
    
    float maxDistSq = findData->maxDistSq;
    float closestDistSq = findData->closestDistSq;
    Entity *closestEntity = findData->entity;
    Vec3 *closestPos = findData->closestPos;

    Vec3 nearPos = { findData->nearPos[0], findData->nearPos[1], findData->nearPos[2] };
    Vec3 *curPos = static_cast<Vec3*>(componentArrays[COMPONENT_INDEX(Position, archetype)]);
    Entity **curEntity = static_cast<Entity**>(componentArrays[0]);
    while (count)
    {
        Vec3 posDiff;
        float curDistSq;
        VEC3_DIFF(posDiff, *curPos, nearPos);
        curDistSq = VEC3_DOT(posDiff, posDiff);

        if (curDistSq < maxDistSq && curDistSq < closestDistSq)
        {
            closestDistSq = curDistSq;
            closestPos = curPos;
            closestEntity = *curEntity;
        }

        count--;
        curPos++;
        curEntity++;
    }

    findData->closestDistSq = closestDistSq;
    findData->closestPos = closestPos;
    findData->entity = closestEntity;
}

Entity *findClosestEntity(Vec3 pos, archetype_t archetype, float maxDist, float *foundDist, Vec3 foundPos)
{
    FindClosestData findData = 
    {
        .nearPos = pos,
        .maxDistSq = std::pow<float, float>(maxDist, 2),
        .closestDistSq = std::numeric_limits<float>::max(),
        .closestPos = nullptr,
        .entity = nullptr,
    };
    // Entities need a position component to be "close" to something, so only look for ones that have it
    archetype |= Bit_Position;
    iterateOverEntitiesAllComponents(findClosestCallback, &findData, archetype, 0);

    if (findData.entity != nullptr)
    {
        *foundDist = std::sqrt(findData.closestDistSq);
        VEC3_COPY(foundPos, *findData.closestPos);
        return findData.entity;
    }

    return nullptr;
}
