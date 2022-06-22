#ifndef __MAIN_H__
#define __MAIN_H__

#include <types.h>

void platformInit(void);

extern uint32_t g_gameTimer; // Counts up every game logic frame (60 fps)
extern uint32_t g_graphicsTimer; // Counters up every graphics frame (dependent on if FPS30 is set)

#endif