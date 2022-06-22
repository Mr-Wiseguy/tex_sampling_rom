#ifndef __N64_INIT_H__
#define __N64_INIT_H__

#include <ultra64.h>
#include <config.h>

#define NUM_THREADS 5

#define IDLE_THREAD 1
#define IDLE_THREAD_INDEX (IDLE_THREAD - 1)
#define IDLE_THREAD_STACKSIZE 0x100

#define AUDIO_THREAD 2
#define AUDIO_THREAD_INDEX (AUDIO_THREAD - 1)
#define AUDIO_THREAD_STACKSIZE 0x4000
#define AUDIO_THREAD_PRI 12

#define MAIN_THREAD 3
#define MAIN_THREAD_INDEX (MAIN_THREAD - 1)
#define MAIN_THREAD_STACKSIZE 0x4000
#define MAIN_THREAD_PRI 10

#define LOAD_THREAD 4
#define LOAD_THREAD_INDEX (LOAD_THREAD - 1)
#define LOAD_THREAD_STACKSIZE 0x1000
#define LOAD_THREAD_PRI 11

#define CRASH_THREAD 5
#define CRASH_THREAD_PRI OS_PRIORITY_APPMAX

#define SCHEDULER_PRI 13

#define NUM_PI_MESSAGES 8
#define NUM_SI_MESSAGES 8

extern "C" void init(void);
void idle(void*);

#endif