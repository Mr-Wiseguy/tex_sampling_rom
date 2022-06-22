#include <cstdint>
#include <cstring>

#include <multiarraylist.h>
#include <mathutils.h>
#include <mem.h>
#include <ecs.h>

// Seems like the best option because it doesn't require a lookup table (slow ram performance)
// or multiplication (delay slots)
// https://stackoverflow.com/questions/757059/position-of-least-significant-bit-that-is-set
int lowest_bit(size_t value)
{
    int i16 = !(value & 0xffff) << 4;
    value >>= i16;

    int i8 = !(value & 0xff) << 3;
    value >>= i8;

    int i4 = !(value & 0xf) << 2;
    value >>= i4;

    int i2 = !(value & 0x3) << 1;
    value >>= i2;

    int i1 = !(value & 0x1);

    return i16 + i8 + i4 + i2 + i1;
}

inline void clear_block(MultiArrayListBlock* block)
{
    uint64_t* cur_ptr = reinterpret_cast<uint64_t*>(block);
    // bzero(block, mem_block_size);
    __asm__ __volatile__(".set gp=64");
    for (size_t i = 0; i < mem_block_size / 8; i++)
    {
        // *cur_ptr = 0;
        __asm__ __volatile__("sd $zero, 0(%0)" : : "r"(cur_ptr));
        cur_ptr++;
    }
    __asm__ __volatile__(".set gp=32");
}

void multiarraylist_init(MultiArrayList *arr, archetype_t archetype)
{
    // Every entity's components has a pointer back to the entity itself
    size_t totalElementSize = sizeof(Entity*);
    archetype_t archetypeShifted;
    int i;

    archetypeShifted = archetype;

    for (i = 0; i < NUM_COMPONENT_TYPES; i++)
    {
        if (archetypeShifted & 0x01)
        {
            totalElementSize += g_componentSizes[i];
        }
        archetypeShifted >>= 1;
    }

    arr->archetype = archetype;
    arr->totalElementSize = totalElementSize;
    arr->elementCount = ROUND_DOWN((mem_block_size - sizeof(MultiArrayListBlock)) / totalElementSize, 4);
    arr->end = arr->start = (MultiArrayListBlock*) allocChunks(1, ALLOC_ECS);
    clear_block(arr->start);
    // memset(arr->start, 0, mem_block_size);
}

void multiarraylist_alloccount(MultiArrayList *arr, size_t count)
{
    size_t elementCount = arr->elementCount;
    size_t remainingInCurrentBlock = elementCount - arr->end->numElements;
    if (count < remainingInCurrentBlock)
    {
        arr->end->numElements += count;
    }
    else
    {
        arr->end->numElements = elementCount;
        count -= remainingInCurrentBlock;
        while (count > 0)
        {
            MultiArrayListBlock *newSeg = (MultiArrayListBlock*) allocChunks(1, ALLOC_ECS);
            clear_block(newSeg);
            // memset(newSeg, 0, mem_block_size);

            arr->end->next = newSeg;
            arr->end = newSeg;
            
            newSeg->numElements = MIN(count, elementCount);
            count -= newSeg->numElements;
        }
    }
}

__attribute__((noinline)) Entity** multiarraylist_get_block_entity_pointers(MultiArrayListBlock *block)
{
    return (Entity**)((uintptr_t)block + sizeof(MultiArrayListBlock));
}

size_t multiarraylist_get_component_offset(MultiArrayList *arr, size_t componentIndex)
{
    archetype_t archetype = arr->archetype;
    size_t elementCount = arr->elementCount;
    // The arrays start after the segment header, and the first array is the pointers to each corresponding entity
    size_t offset = sizeof(MultiArrayListBlock) + elementCount * sizeof(Entity*);
    size_t curComponentType;

    // Iterate over each component in the archetype until we reach the provided one
    // If the current component being iterated over isn't the provided one, increase the offset by the component's array size
    while ((curComponentType = lowest_bit(archetype)) != componentIndex)
    {
        offset += g_componentSizes[curComponentType] * elementCount;
        archetype &= ~(1 << curComponentType);
    }
    return offset;
}

void multiarraylist_delete(MultiArrayList *arr, size_t arrayIndex)
{
    archetype_t archetype = arr->archetype;
    size_t elementCount = arr->elementCount;
    MultiArrayListBlock *block = arr->start;
    MultiArrayListBlock *end = arr->end;
    size_t block_array_index = arrayIndex;

    // Find the block
    while (block_array_index >= elementCount)
    {
        block = block->next;
        block_array_index -= elementCount;
    }

    // Copy the components of the last element in the array to the position of the deleted one, but only
    // if the deleted entity is not the last in the multi array list
    if (!(block == end && block_array_index == end->numElements - 1))
    {
        Entity** end_block_entities = multiarraylist_get_block_entity_pointers(end);
        Entity* repointed_entity = end_block_entities[end->numElements - 1];

        // Copy the entity pointer from the last element in the last block's components to the deleted entity's components
        multiarraylist_get_block_entity_pointers(block)[block_array_index] = repointed_entity;
        // Update the repointed entity's component index
        repointed_entity->archetypeArrayIndex = arrayIndex;

        // Swap the last element's components into the position of the deleted element's components
        size_t current_array_offset = sizeof(MultiArrayListBlock) + elementCount * sizeof(Entity*);

        // Iterate over every component in the archetype
        archetype_t component_bits = archetype;
        while (component_bits != 0)
        {
            // Get the component type (global component index) of the next component in the component bits
            size_t cur_component_type = lowest_bit(component_bits);
            // Get the size of the current component type
            size_t cur_component_size = g_componentSizes[cur_component_type];
            // Get the addresses of the component to delete and the component to replace it with
            void *deletedComponent = (void *)((uintptr_t)block + current_array_offset + cur_component_size * block_array_index);
            void *endComponent = (void *)((uintptr_t)end + current_array_offset + cur_component_size * (end->numElements - 1));
            // Replace the contents of the current component
            memcpy(deletedComponent, endComponent, cur_component_size);
            // Update the offset for the array for the next component
            current_array_offset += cur_component_size * elementCount;
            // Clear the current component from the component bits
            component_bits &= ~(1 << cur_component_type);
        }
    }

    // Decrement the number of elements in the last block
    end->numElements--;

    // If the last block has no more elements in it, make the previous node the new end
    if (end->numElements == 0 && arr->start != arr->end)
    {
        MultiArrayListBlock *newEnd = arr->start;
        while (newEnd->next != end)
        {
            newEnd = newEnd->next;
        }
        arr->end = newEnd;
        newEnd->next = nullptr;
        end = newEnd;
    }
}
