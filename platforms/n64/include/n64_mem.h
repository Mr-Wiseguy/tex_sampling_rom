#ifndef __N64_MEM_H__
#define __N64_MEM_H__

#include <types.h>
#include <PR/ultratypes.h>
#include <PR/os_pi.h>

extern OSPiHandle *g_romHandle;
extern OSMesgQueue siMesgQueue;
extern uint8_t memPoolStart[];

extern uint8_t _introSegmentRomStart[];
extern uint8_t _introSegmentRomEnd[];
extern uint8_t _creditsSegmentRomStart[];
extern uint8_t _creditsSegmentRomEnd[];

extern uint8_t _mainSegmentBssStart[];
extern uint8_t _mainSegmentBssEnd[];

void setSegment(uint32_t segmentIndex, void* virtualAddress);
void *getSegment(uint32_t segmentIndex);

void startDMA(void *targetVAddr, void *romStart, int length);
void waitForDMA();

#endif
