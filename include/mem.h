#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <types.h>

// #define USE_EXT_RAM
#ifdef USE_EXT_RAM
#define MEM_END 0x80800000
#else
#define MEM_END 0x80400000
#endif

typedef uint8_t owner_t;

// Ids for what system allocated a chunk (owners)
#define ALLOC_FREE       0
#define ALLOC_GFX        1
#define ALLOC_AUDIO      2
#define ALLOC_ECS        3
#define ALLOC_FILE       4

#define ALLOC_MALLOC     252 // Memory allocated by malloc
#define ALLOC_NEW        253 // Memory allocated by new
#define ALLOC_NEW_ARR    254 // Memory allocated by new[]
#define ALLOC_CONTIGUOUS 255 // Allocation from previous chunk

constexpr size_t mem_block_size = 1024;
constexpr size_t mem_small_block_size = 256; // TODO: implement small chunks in memory pool
#define SEGMENT_COUNT 32
#define ROUND_UP(val, multiple) (((val) + (multiple) - 1) & ~((multiple) - 1))
#define ROUND_DOWN(val, multiple) (((val) / (multiple)) * (multiple))

uint32_t isSegmented(void* segmented);
void* segmentedToVirtual(void* segmented);

template <typename T>
T* segmentedToVirtual(T* segmented)
{
    return static_cast<T*>(segmentedToVirtual(static_cast<void*>(segmented)));
}

// Set up the memory allocation parameters
void initMemAllocator(void *start, void *end);

// Allocates a given number of contiguous memory chunks, each of size mem_block_size
void *allocChunks(int numChunks, owner_t owner);
// Allocates a contiguous region of memory at least as large as the given length
void *allocRegion(int length, owner_t owner);
// Free a region of allocated memory
void freeAlloc(void *start) noexcept;

// Deleter class for use with unique_ptr when holding memory allocated with allocRegion/allocChunks
class alloc_deleter
{
public:
    void operator()(void* ptr) { freeAlloc(ptr); }
};

#endif
