#include <physics.h>
#include <config.h>
#include <mathutils.h>
#include <ecs.h>
#include <collision.h>

#include <n64_mathutils.h>

#include <memory>

extern "C" {
#include <debug.h>
}

#define ARCHETYPE_POSVEL        (Bit_Position | Bit_Velocity)
#define ARCHETYPE_GRAVITY       (Bit_Velocity | Bit_Gravity)
#define ARCHETYPE_VEL_COLLIDER  (Bit_Position | Bit_Velocity | Bit_Collider)
#define ARCHETYPE_DEACTIVATABLE (Bit_Position | Bit_Deactivatable)

void applyGravityImpl(size_t count, Vec3* cur_vel, GravityParams* gravity, ActiveState* active_state)
{
    while (count)
    {
        if (active_state == nullptr || !active_state->deactivated)
        {
            (*cur_vel)[1] += gravity->accel;

            if ((*cur_vel)[1] < gravity->terminalVelocity)
            {
                (*cur_vel)[1] = gravity->terminalVelocity;
            }
            
        }

        if (active_state != nullptr)
        {
            active_state++;
        }
        
        cur_vel++;
        gravity++;
        count--;
    }
}

void applyGravityCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    // Components: Velocity, Gravity
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_GRAVITY);
    GravityParams *gravity =  get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_GRAVITY);
    applyGravityImpl(count, cur_vel, gravity, nullptr);
}

void applyGravityDeactivatableCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_GRAVITY | Bit_Deactivatable);
    GravityParams *gravity =  get_component<Bit_Gravity, GravityParams>(componentArrays, ARCHETYPE_GRAVITY | Bit_Deactivatable);
    ActiveState *active_state = get_component<Bit_Deactivatable, ActiveState>(componentArrays, ARCHETYPE_GRAVITY | Bit_Deactivatable);
    applyGravityImpl(count, cur_vel, gravity, active_state);
}

void applyVelocityImpl(size_t count, Vec3* cur_pos, Vec3* cur_vel, ActiveState* active_state)
{
    while (count)
    {
        if (active_state == nullptr || !active_state->deactivated)
        {
            VEC3_ADD(*cur_pos, *cur_pos, *cur_vel);
        }

        if (active_state)
        {
            active_state++;
        }

        cur_pos++;
        cur_vel++;
        count--;
    }
}

void applyVelocityCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    Vec3 *cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_POSVEL);
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_POSVEL);
    applyVelocityImpl(count, cur_pos, cur_vel, nullptr);
}

void applyVelocityDeactivatableCallback(size_t count, UNUSED void *arg, void **componentArrays)
{
    Vec3 *cur_pos = get_component<Bit_Position, Vec3>(componentArrays, ARCHETYPE_POSVEL | Bit_Deactivatable);
    Vec3 *cur_vel = get_component<Bit_Velocity, Vec3>(componentArrays, ARCHETYPE_POSVEL | Bit_Deactivatable);
    ActiveState *active_state = get_component<Bit_Deactivatable, ActiveState>(componentArrays, ARCHETYPE_POSVEL | Bit_Deactivatable);
    applyVelocityImpl(count, cur_pos, cur_vel, active_state);
}

void physicsTick()
{
    // Apply gravity to all objects that cannot be deactivated and are affected by it
    iterateOverEntities(applyGravityCallback, nullptr, ARCHETYPE_GRAVITY, Bit_Deactivatable);
    // Apply gravity to all objects that can be deactivated and are affected by it
    iterateOverEntities(applyGravityDeactivatableCallback, nullptr, ARCHETYPE_GRAVITY | Bit_Deactivatable, 0);
    // Apply every non-deactivatable object's velocity to their position
    iterateOverEntities(applyVelocityCallback, nullptr, ARCHETYPE_POSVEL, Bit_Deactivatable);
    // Apply every deactivatable object's velocity to their position
    iterateOverEntities(applyVelocityDeactivatableCallback, nullptr, ARCHETYPE_POSVEL | Bit_Deactivatable, 0);
}
