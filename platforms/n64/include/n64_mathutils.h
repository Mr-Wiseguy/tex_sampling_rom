#ifndef __N64_MATHUTILS_H__
#define __N64_MATHUTILS_H__

inline int32_t lround(float x)
{
    float retf;
    int ret;
    __asm__ __volatile__("round.w.s %0, %1" : "=f"(retf) : "f"(x));
    __asm__ __volatile__("mfc1 %0, %1" : "=r"(ret) : "f"(retf));
    return ret;
}

inline int32_t lfloor(float x)
{
    float retf;
    int ret;
    __asm__ __volatile__("floor.w.s %0, %1" : "=f"(retf) : "f"(x));
    __asm__ __volatile__("mfc1 %0, %1" : "=r"(ret) : "f"(retf));
    return ret;
}

inline int32_t lceil(float x)
{
    float retf;
    int ret;
    __asm__ __volatile__("ceil.w.s %0, %1" : "=f"(retf) : "f"(x));
    __asm__ __volatile__("mfc1 %0, %1" : "=r"(ret) : "f"(retf));
    return ret;
}

#endif
