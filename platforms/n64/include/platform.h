#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <cstdint>
#include <PR/R4300.h>

// Copies from os_cont.h to prevent bleeding any libultra typedefs
/* Buttons */

#define CONT_A      0x8000
#define CONT_B      0x4000
#define CONT_G	    0x2000
#define CONT_START  0x1000
#define CONT_UP     0x0800
#define CONT_DOWN   0x0400
#define CONT_LEFT   0x0200
#define CONT_RIGHT  0x0100
#define CONT_L      0x0020
#define CONT_R      0x0010
#define CONT_E      0x0008
#define CONT_D      0x0004
#define CONT_C      0x0002
#define CONT_F      0x0001

/* Nintendo's official button names */

#define A_BUTTON	CONT_A
#define B_BUTTON	CONT_B
#define L_TRIG		CONT_L
#define R_TRIG		CONT_R
#define Z_TRIG		CONT_G
#define START_BUTTON	CONT_START
#define U_JPAD		CONT_UP
#define L_JPAD		CONT_LEFT
#define R_JPAD		CONT_RIGHT
#define D_JPAD		CONT_DOWN
#define U_CBUTTONS	CONT_E
#define L_CBUTTONS	CONT_C
#define R_CBUTTONS	CONT_F
#define D_CBUTTONS	CONT_D

#ifdef __cplusplus

// C++ stuff in the global namespace here
void platformInit();

namespace n64 {
#endif

// static inline functions here

static inline void disableInterrupts() {
    uint32_t statusReg;
    __asm__ __volatile__("mfc0 %0, $%1" : "=r"(statusReg) : "I"(C0_SR));
    statusReg &= ~SR_IE;
    __asm__ __volatile__("mtc0 %0, $%1" : : "r"(statusReg), "I"(C0_SR));
}

static inline void enableInterrupts() {
    uint32_t statusReg;
    __asm__ __volatile__("mfc0 %0, $%1" : "=r"(statusReg) : "I"(C0_SR));
    statusReg |= SR_IE;
    __asm__ __volatile__("mtc0 %0, $%1" : : "r"(statusReg), "I"(C0_SR));
}

#ifdef __cplusplus

// C++ stuff here

// Simple "mutex" that just disables interrupts to prevent preemption, since the N64 is a single CPU system
class ultra_mutex {
public:
    ultra_mutex() {}
    void lock() { disableInterrupts(); }
    void unlock() { enableInterrupts(); }
    ultra_mutex(const ultra_mutex&) = delete;
    ultra_mutex& operator=(const ultra_mutex&) = delete;
};

} // namespace n64

namespace std {
    // libstdc++ is compiled without threads on N64, so use our custom "mutex" as std::mutex
    using mutex = n64::ultra_mutex;
} // namespace std

extern "C"
{
#endif
// C stuff here

#ifdef __cplusplus
} // extern "C"
#endif

#endif
