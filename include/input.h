#ifndef __INPUT_H__
#define __INPUT_H__

#include <types.h>

#define INPUT_DEADZONE 0.1f

enum class InputType : uint8_t {
    None,
    Mouse,
    SingleAnalog,
    DualAnalog
};

typedef struct InputData_t {
    float x;
    float y;
    float r_x;
    float r_y;
    float l_trig;
    float r_trig;
    float magnitude;
    int16_t angle;
    uint16_t buttonsHeld;
    uint16_t buttonsPressed;
    InputType type;
} InputData;

inline InputData g_InputData[4];

void initInput();
void beginInputPolling();
void readInput();
void start_rumble(int channel);
void stop_rumble(int channel);


#endif
