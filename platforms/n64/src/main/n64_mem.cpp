#include <n64_mem.h>

#include <ultra64.h>

#include <mem.h>

extern "C" void bzero(void*, unsigned int);
extern "C" void bcopy(const void*, void*, unsigned int);

OSMesgQueue dmaMesgQueue;

OSMesg dmaMessage;

OSIoMesg dmaIoMessage;

void startDMA(void *targetVAddr, void *romAddr, int length)
{
    // Zero out the region being DMA'd to
    bzero(targetVAddr, length);

    // Create message queue for DMA reads/writes
    osCreateMesgQueue(&dmaMesgQueue, &dmaMessage, 1);
    
    // Invalidate the data cache for the region being DMA'd to
    osInvalDCache(targetVAddr, length); 

    // Set up the intro segment DMA
    dmaIoMessage.hdr.pri = OS_MESG_PRI_NORMAL;
    dmaIoMessage.hdr.retQueue = &dmaMesgQueue;
    dmaIoMessage.dramAddr = targetVAddr;
    dmaIoMessage.devAddr = (u32)romAddr;
    dmaIoMessage.size = (u32)length;

    // Start the DMA
    osEPiStartDma(g_romHandle, &dmaIoMessage, OS_READ);
}

void waitForDMA()
{
    // Wait for the DMA to complete
    osRecvMesg(&dmaMesgQueue, nullptr, OS_MESG_BLOCK);
}

uintptr_t segmentTable[SEGMENT_COUNT];

void setSegment(u32 segmentIndex, void* virtualAddress)
{
    segmentTable[segmentIndex] = (uintptr_t) virtualAddress;
}

void *getSegment(uint32_t segmentIndex)
{
    return (void *)segmentTable[segmentIndex];
}

void* segmentedToVirtual(void* segmentedAddress)
{
    u32 segmentIndex = (uintptr_t)segmentedAddress >> 24;
    if (segmentIndex == 0x80)
        return segmentedAddress;
    return (void*)(segmentTable[segmentIndex] + ((uintptr_t)segmentedAddress & 0xFFFFFF));
}

extern "C" void abort()
{
    *(volatile int*)0 = 0;
    while (1);
}

extern "C" void* memset(void* ptr, int value, size_t num)
{
    uint8_t val_8 = static_cast<uint8_t>(value);
    uint32_t val_word = 
        (val_8 << 24) |
        (val_8 << 16) |
        (val_8 <<  8) |
        (val_8);

    uintptr_t bytes_end = reinterpret_cast<uintptr_t>(ptr) + num;
    uintptr_t words_end = bytes_end & ~(0b11);
    uintptr_t ptr_uint = reinterpret_cast<uintptr_t>(ptr);

    while (ptr_uint & 0b11)
    {
        *reinterpret_cast<uint8_t*>(ptr_uint) = value;
        ptr_uint += 1;
    }

    while (ptr_uint != words_end)
    {
        *reinterpret_cast<uint32_t*>(ptr_uint) = val_word;
        ptr_uint += 4;
    }

    while (ptr_uint != bytes_end)
    {
        *reinterpret_cast<uint8_t*>(ptr_uint) = value;
        ptr_uint += 1;
    }

	return ptr;
}

extern "C" int strncmp (const char *s1, const char *s2, size_t n) {
    while ( n && *s1 && ( *s1 == *s2 ) )
    {
        ++s1;
        ++s2;
        --n;
    }
    if ( n == 0 )
    {
        return 0;
    }
    else
    {
        return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
    }
}

extern "C" void* memmove(void* destination, const void* source, size_t num) {
    bcopy(source, destination, num);
    return destination;
}

// extern "C" double log2(double x)
// {
//   union { float f; uint32_t i; } vx = { (float)x };
//   union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
//   float y = vx.i;
//   y *= 1.1920928955078125e-7f;

//   return y - 124.22551499f
//            - 1.498030302f * mx.f 
//            - 1.72587999f / (0.3520887068f + mx.f);
// }


// #ifdef __cplusplus
// #define cast_uint32_t static_cast<uint32_t>
// #else
// #define cast_uint32_t (uint32_t)
// #endif

// float fastpow2 (float p)
// {
//   float clipp = (p < -126) ? -126.0f : p;
//   union { uint32_t i; float f; } v = { cast_uint32_t ( (1 << 23) * (clipp + 126.94269504f) ) };
//   return v.f;
// }

// static inline float 
// fastlog2 (float x)
// {
//   union { float f; uint32_t i; } vx = { x };
//   union { uint32_t i; float f; } mx = { (vx.i & 0x007FFFFF) | 0x3f000000 };
//   float y = vx.i;
//   y *= 1.1920928955078125e-7f;

//   return y - 124.22551499f
//            - 1.498030302f * mx.f 
//            - 1.72587999f / (0.3520887068f + mx.f);
// }

// extern "C" float powf (float x, float p)
// {
//   return fastpow2 (p * fastlog2 (x));
// }
