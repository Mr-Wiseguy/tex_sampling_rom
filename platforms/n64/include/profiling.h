#ifndef __PROFILING_H__
#define __PROFILING_H__

#include <ultra64.h>
#include <types.h>

// #define USE_PROFILER

#define PROFILING_BUFFER_SIZE 128

enum ProfilerTime {
    PROFILER_TIME_FPS,
    PROFILER_TIME_SETUP,
    // PROFILER_TIME_PHYSICS,
    // PROFILER_TIME_COLLISIONS,
    // PROFILER_TIME_BHV,
    PROFILER_TIME_UPDATE,
    PROFILER_TIME_CONTROLLERS,
    // PROFILER_TIME_PHYSICS2,
    // PROFILER_TIME_COLLISIONS2,
    // PROFILER_TIME_BHV2,
    PROFILER_TIME_UPDATE2,
    PROFILER_TIME_CONTROLLERS2,
    PROFILER_TIME_GFX,
    PROFILER_TIME_AUDIO,
    PROFILER_TIME_TOTAL,
    PROFILER_TIME_TMEM,
    PROFILER_TIME_PIPE,
    PROFILER_TIME_CMD,
    PROFILER_TIME_RSP_GFX,
    PROFILER_TIME_RSP_AUDIO,
    PROFILER_TIME_COUNT,
};

enum ProfilerRSPTime {
    PROFILER_RSP_GFX,
    PROFILER_RSP_AUDIO,
    PROFILER_RSP_COUNT
};

#ifdef USE_PROFILER
void profiler_update(enum ProfilerTime which);
void profiler_print_times();
void profiler_frame_setup();
void profiler_rsp_started(enum ProfilerRSPTime which);
void profiler_rsp_completed(enum ProfilerRSPTime which);
void profiler_rsp_resumed();
void profiler_audio_started();
void profiler_audio_completed();
// See profiling.c to see why profiler_rsp_yielded isn't its own function
static FORCEINLINE void profiler_rsp_yielded() {
    profiler_rsp_resumed();
}
#else
static inline void profiler_update(UNUSED enum ProfilerTime which) {}
static inline void profiler_print_times() {}
static inline void profiler_frame_setup() {}
static inline void profiler_rsp_started(UNUSED enum ProfilerRSPTime which) {}
static inline void profiler_rsp_completed(UNUSED enum ProfilerRSPTime which) {}
static inline void profiler_rsp_resumed() {}
static inline void profiler_audio_started() {}
static inline void profiler_audio_completed() {}
static inline void profiler_rsp_yielded() {}
#endif

#endif
