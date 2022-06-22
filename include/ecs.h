#ifndef __ECS_H__
#define __ECS_H__

#include <types.h>
#include <platform_gfx.h>

#define MAX_ARCHETYPES 256

// TODO dynamically allocate entity memory
#define MAX_ENTITIES 65536

#define COMPONENT(Name, Type) Component_##Name,

enum Components
{
#include "components.inc.h"
NUM_COMPONENT_TYPES
};

#undef COMPONENT

#define COMPONENT(Name, Type) Bit_##Name = 1 << Component_##Name,

enum ComponentBits
{
#include "components.inc.h"
};

#undef COMPONENT

#define NUM_COMPONENTS(Archetype) (__builtin_popcount(Archetype))
#define COMPONENT_INDEX(Component, Archetype) (1 + (__builtin_popcount((Archetype) & (Bit_##Component - 1))))

#define ARCHETYPE_MODEL (Bit_Position | Bit_Rotation | Bit_Model)
#define ARCHETYPE_MODEL_NO_ROTATION (Bit_Position | Bit_Model)
#define ARCHETYPE_SCALED_MODEL (Bit_Position | Bit_Rotation | Bit_Model | Bit_Scale)
#define ARCHETYPE_ANIM_MODEL (Bit_Position | Bit_Rotation | Bit_Model | Bit_AnimState)
#define ARCHETYPE_SCALED_ANIM_MODEL (Bit_Position | Bit_Rotation | Bit_Model | Bit_AnimState | Bit_Scale)

typedef struct Entity_t {
    // The archetype of this entity
    archetype_t archetype;
    // The index of this entity in the archetype's arraylist
    size_t archetypeArrayIndex;
} Entity;

// Callback provided to the ecs to be called for every array of a given component selection when iterating
typedef void (*EntityArrayCallback)(size_t count, void *arg, void **componentArrays);

// Callback provided to the ecs to be called for every array of a given component selection when iterating
// This callback is passed ALL components in each entity, not just the ones passed in the mask
typedef void (*EntityArrayCallbackAll)(size_t count, void *arg, int numComponents, archetype_t archetype, void **componentArrays, size_t *componentSizes);

// Callback for entities that have a behavior component
// First argument passed is an array of the component pointers
// Second argument is the value of data in the behavior parameters
typedef void (*EntityBehaviorCallback)(void **components, void *data);

struct BehaviorState
{
    EntityBehaviorCallback callback;
    std::array<uint8_t, 16> data;
};

struct ActiveState {
    uint8_t delete_on_deactivate : 1;
    uint8_t deactivated : 1;
};

#include <bit>
#include <memory>

template <unsigned int ComponentBit, typename ComponentType>
constexpr ComponentType* get_component(void **components, archetype_t archetype)
{
    return static_cast<ComponentType*>(components[1 + std::popcount(archetype & (ComponentBit - 1))]);
}

template <unsigned int ComponentBit, typename ComponentType>
constexpr ComponentType* get_component(const std::unique_ptr<void*[]>& components, archetype_t archetype)
{
    return get_component<ComponentBit, ComponentType>(components.get(), archetype);
}

constexpr Entity* get_entity(void **components)
{
    return *static_cast<Entity**>(components[0]);
}


// Creates a single entity, try to avoid using as creating entities in batches is more efficient
Entity *createEntity(archetype_t archetype);
// Deletes a single entity
void deleteEntity(Entity *e);
// Creates a number of entities
void createEntities(archetype_t archetype, int count);
// Creates a number of entities, and calls the provided callback for each block of allocated entities
void createEntitiesCallback(archetype_t archetype, void *arg, int count, EntityArrayCallback callback);

// Calls the given callback for each array that fits the given archetype and does not fit the reject archetype
void iterateOverEntities(EntityArrayCallback callback, void *arg, archetype_t componentMask, archetype_t rejectMask);
// Same as above, but this time an array of ALL components is passed (not just the masked ones), as well as an array of component sizes
void iterateOverEntitiesAllComponents(EntityArrayCallbackAll callback, void *arg, archetype_t componentMask, archetype_t rejectMask);
// Used to delete an entity during entity iteration
// Queued entities will all get deleted after the current iteration is over
void queue_entity_deletion(Entity*);
// Used to create entities during entity iteration
// Queued entities will all get created after the current iteration is over
void queue_entity_creation(archetype_t archetype, void* arg, int count, EntityArrayCallback callback);
// Registers a new archetype
void registerArchetype(archetype_t archetype);
// Outputs the component pointers for the given entity into the provided pointer array
void getEntityComponents(Entity *entity, void **componentArrayOut);

// Finds the entity that has the given archetype and the given archetype array index
Entity *findEntity(archetype_t archetype, size_t archetypeArrayIndex);
// Deletes all entities (duh)
void deleteAllEntities(void);
// Iterates over every behavior entity and processes their behavior
void iterateBehaviorEntities(void);
// Iterates over every entity with a destroy timer, ticks it, and deletes the entity if the timer reached zero
void tickDestroyTimers(void);

extern const size_t g_componentSizes[];

#endif
