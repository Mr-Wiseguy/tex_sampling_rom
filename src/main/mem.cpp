#include <mem.h>
#include <cstdint>
#include <cstring>

#include <iterator>
#include <array>
// #include <span>

extern "C" {
#include <debug.h>
}

#include <platform.h>
// #include <mutex>
#include <vassert.h>

/**
 * One block of memory in the memory pool, contains links to the previous and next free blocks
 */
class MemoryBlock {
private:
    MemoryBlock *_prevFree;
    MemoryBlock *_nextFree;
    size_t _index;
public:
    struct Iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = MemoryBlock;
        using pointer           = MemoryBlock*;
        using reference         = MemoryBlock&;

        Iterator(pointer ptr) : _ptr(ptr) {}
        reference operator*() const { return *_ptr; }
        pointer operator->() { return _ptr; }
        Iterator& operator++() { _ptr = _ptr->_nextFree; return *this; }
        Iterator& operator--() { _ptr = _ptr->_prevFree; return *this; }
        Iterator operator++(int) { Iterator tmp = *this; tmp._ptr = tmp._ptr->_nextFree; return tmp; }
        Iterator operator--(int) { Iterator tmp = *this; tmp._ptr = tmp._ptr->_prevFree; return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a._ptr == b._ptr; }
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a._ptr != b._ptr; }
    private:
        pointer _ptr;
    };
    MemoryBlock(MemoryBlock *prevFree, MemoryBlock *nextFree, size_t index) :
        _prevFree(prevFree), _nextFree(nextFree), _index(index) {}
    MemoryBlock(size_t index) :
        _index(index) {}

    Iterator begin() { return Iterator(this); }
    Iterator end() { return Iterator(nullptr); }
    
    size_t index() { return _index; }
    MemoryBlock *unlink();
    void insert_link(MemoryBlock *newPrev);
};

// Links this block's previous and next blocks together, removing this one from the list
// Returns what was previously the next block in the list
MemoryBlock *MemoryBlock::unlink()
{
    if (_prevFree != nullptr)
        _prevFree->_nextFree = _nextFree;
    if (_nextFree != nullptr)
        _nextFree->_prevFree = _prevFree;
    return _nextFree;
}

// Links a block into the list, directly before this block.
// Sets this block's previous to the new block, the prior previous block's next to the new block,
// the new block's previous to the prior previous block, and the new block's next to this block.
void MemoryBlock::insert_link(MemoryBlock *newPrev)
{
    MemoryBlock *oldPrev = _prevFree;
    if (oldPrev != nullptr)
    {
        oldPrev->_nextFree = newPrev;
    }
    newPrev->_prevFree = oldPrev;
    newPrev->_nextFree = this;
    _prevFree = newPrev;
}

class MemoryPool {
private:
    // Pointer to the start of an array with an owner_t for each chunk representing
    // what it was allocated by, or 0 if free
    owner_t *_blockTable;
    // The address of the first memory block
    uintptr_t _blocksStart;
    // The number of memory chunks
    size_t _totalBlocks;
    // First free chunk in the free chunk chain
    MemoryBlock *_firstFree;
public:
    MemoryPool() = default;
    MemoryPool(void *start, void *end);
    size_t index_from_block(MemoryBlock *t);
    MemoryBlock *block_from_index(size_t index);
    void *alloc(int num_blocks, owner_t owner);
    void free(void *mem) noexcept;
};

MemoryPool::MemoryPool(void *start, void *end)
{
    // Calculate the number of chunks in the available memory
    // This is equal to the available memory divided by the chunk size plus the size of an owner, since
    // an extra owner value is needed for each chunk in the chunk table
    _totalBlocks = ((uintptr_t)end - (uintptr_t)start) / (mem_block_size + sizeof(owner_t));
    _blocksStart = (uintptr_t)end - (_totalBlocks * mem_block_size);
    _blockTable = static_cast<owner_t*>(start);

    MemoryBlock *lastBlock = nullptr;
    MemoryBlock *curBlock = block_from_index(0);
    MemoryBlock *nextBlock; // = block_from_index(1);
    // Set up the linked list
    _firstFree = curBlock;

    // Initialize all but the last memory blocks
    for (size_t curBlockIndex = 0; curBlockIndex < _totalBlocks - 1; curBlockIndex++)
    {
        nextBlock = block_from_index(curBlockIndex + 1);
        new (curBlock) MemoryBlock(lastBlock, nextBlock, curBlockIndex);
        lastBlock = curBlock;
        curBlock = nextBlock;
    }
    // Initialize the last memory block
    new (curBlock) MemoryBlock(lastBlock, nullptr, _totalBlocks - 1);
    // Clear the block ownership table
    memset(_blockTable, ALLOC_FREE, _totalBlocks);
}

// Calculates a block's index from its address
size_t MemoryPool::index_from_block(MemoryBlock *block)
{
    uintptr_t blockAddr = uintptr_t(block);
    return (blockAddr - _blocksStart) / mem_block_size;
}

// Calculates a block's address from its index
MemoryBlock *MemoryPool::block_from_index(size_t index)
{
    uintptr_t blockAddr = _blocksStart + index * mem_block_size;
    return (MemoryBlock *)blockAddr;
}

// std::mutex mem_mutex{};

// Allocates a contiguous number of blocks with the given owner
void *MemoryPool::alloc(int num_blocks, owner_t owner)
{
    // std::lock_guard guard(mem_mutex);
    // No free chunks, return nullptr
    if (_firstFree == nullptr)
        return nullptr;

    vassert(num_blocks != 0,
        "Attempted to allocate zero blocks\nOwner: %d", owner);

    // Only allocating 1 chunk, simply return the first free one and update the first free chunk index
    // TODO fix this, zero byte allocations should never be happening
    if (num_blocks == 1)
    {
        MemoryBlock *retBlock = _firstFree;
        UNUSED owner_t prev_owner = _blockTable[index_from_block(retBlock)];
        vassert(prev_owner == ALLOC_FREE,
            "Double alloc at %08X\nAlready claimed by owner %d", retBlock, prev_owner);
        _firstFree = _firstFree->unlink();
        _blockTable[retBlock->index()] = owner;
        
        // debug_printf("Allocated %08X\n", retBlock);
        return retBlock;
    }
    else
    {
        // Iterate over every free block to find a large enough contiguous region of free blocks
        for (auto &freeBlock : *_firstFree)
        {
            // Get the index of the current free block to check if the blocks directly after it are free
            size_t freeBlockIndex = freeBlock.index();
            // Determine the end of the current required contiguous blocks
            size_t endBlockIndex = freeBlockIndex + num_blocks;
            // Record if the blocks were all free
            int allocSuccessful = true;
            for (size_t checkedBlockIndex = freeBlockIndex + 1; checkedBlockIndex < endBlockIndex; checkedBlockIndex++)
            {
                // At the end of available memory, not enough room to allocate the given number of blocks
                // or
                // A long enough contiguous region of free blocks is not available starting at the current free block
                if (checkedBlockIndex >= _totalBlocks || _blockTable[checkedBlockIndex] != ALLOC_FREE)
                {
                    allocSuccessful = false;
                    break;
                }
            }
            // If we found a valid region to allocate, allocate it
            if (allocSuccessful)
            {
                owner_t updatedOwner = owner;

                // Allocate the region
                for (size_t curBlockIndex = freeBlockIndex; curBlockIndex < endBlockIndex; curBlockIndex++)
                {
                    // Get the current block from its index
                    MemoryBlock *curBlock = block_from_index(curBlockIndex);
                    // Unlink the current block from the free block chain
                    MemoryBlock *newLink = curBlock->unlink();
                    // Update the owner of the current block
                    _blockTable[curBlockIndex] = updatedOwner;
                    // Make every block besides the first one owned by a contiguous allocation,
                    // since only the first block in a contiguous allocation has the actual owner
                    updatedOwner = ALLOC_CONTIGUOUS;
                    // If we allocated the first free block, find a new first free block
                    if (curBlock == _firstFree)
                        _firstFree = newLink;
                }

                // debug_printf("Allocated %08X\n", freeBlock);
                return &freeBlock;
            }
        }
    }
    return nullptr;
}

// Frees a previously allocated block(s)
void MemoryPool::free(void *mem) noexcept
{
    // std::lock_guard guard(mem_mutex);
    // debug_printf("Freeing alloc %08X\n", mem);
    // Cast the input memory address to a MemoryBlock
    MemoryBlock *toFree = static_cast<MemoryBlock*>(mem);
    // Get the index of the block being freed
    size_t toFreeIndex = index_from_block(toFree);
    // Free any blocks that are part of the start block's allocation
    if (_blockTable[toFreeIndex] == ALLOC_FREE)
    {
        // Double free
        // TODO there's one lingering double free somewhere; fix it and put this assert back
        return;
        // *(volatile uint8_t*)toFreeIndex = 0;
    }
    do 
    {
        // Reinitialize the MemoryBlock with its index and no linked blocks
        new (toFree) MemoryBlock(nullptr, nullptr, toFreeIndex);
        // If the list is valid, update it
        if (_firstFree != nullptr)
        {
            // Insert this block at the start of the free block list
            _firstFree->insert_link(toFree);
        }
        // Update the ownership table
        _blockTable[toFreeIndex] = ALLOC_FREE;
        // Update the start of the free block list with the current block
        _firstFree = toFree;
        // Move on to the next block
        toFreeIndex++;
        // Get the next block to be freed
        toFree = block_from_index(toFreeIndex);
    // Continue freeing blocks that are marked as being from a contiguous allocation
    } while (_blockTable[toFreeIndex] == ALLOC_CONTIGUOUS);
}

// Global MemoryPool object
MemoryPool g_memoryPool;

// Initializes the memory allocation settings (chunk table, number of chunks, first chunk address)
void initMemAllocator(void *start, void *end)
{
    // Call placement new on the global MemoryPool to initialize it with the given values
    new (&g_memoryPool) MemoryPool(start, end);
}

void *allocChunks(int numChunks, owner_t owner)
{
    return g_memoryPool.alloc(numChunks, owner);
}

void *allocRegion(int length, owner_t owner)
{
    // Rounded up integer division: (x + (y - 1)) / y
    return g_memoryPool.alloc((length + (mem_block_size - 1)) / mem_block_size, owner);
}

void freeAlloc(void *start) noexcept
{
    g_memoryPool.free(start);
}

void* operator new(size_t sz)
{
    void *ret = allocRegion(sz, ALLOC_NEW);
    if (ret == nullptr)
    {
#if __cpp_exceptions
        throw std::bad_alloc();
#else
        abort();
#endif
    }
    return ret;
}

void *operator new[](size_t sz)
{
    void *ret = allocRegion(sz, ALLOC_NEW_ARR);
    if (ret == nullptr)
    {
#if __cpp_exceptions
        throw std::bad_alloc();
#else
        abort();
#endif
    }
    return ret;
}

void operator delete(void* ptr)
{
    freeAlloc(ptr);
}

void operator delete(void* ptr, size_t)
{
    freeAlloc(ptr);
}

void operator delete[](void* ptr)
{
    freeAlloc(ptr);
}

void operator delete[](void* ptr, size_t)
{
    freeAlloc(ptr);
}

