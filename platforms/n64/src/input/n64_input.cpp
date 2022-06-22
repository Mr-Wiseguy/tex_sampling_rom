#include <ultra64.h>

#include <n64_mem.h>
#include <mathutils.h>
#include <mem.h>
#include <input.h>

typedef struct {
	u16     button;
	s8      stick_x;		/* -80 <= stick_x <= 80 */
	s8      stick_y;		/* -80 <= stick_y <= 80 */
	s8      c_stick_x;
	s8      c_stick_y;
	u8      l_trig;
	u8      r_trig;
	u8	errno;
} OSContPadEx;

#define CONT_GCN 8

// extern "C" void osContGetReadData(OSContPadEx* data);

static OSContStatus controllerStatuses[MAXCONTROLLERS];
// static OSContPadEx contPads[MAXCONTROLLERS];
static OSContPad contPads[MAXCONTROLLERS];
s32 g_HasEeprom;

OSPfs g_Pfs[MAXCONTROLLERS];

void set_input_type(InputData* input, u32 contType, int present) {
    if (!present) {
        input->type = InputType::None;
    } else {
        switch (contType) {
            case CONT_ABSOLUTE | CONT_GCN:
                input->type = InputType::DualAnalog;
                break;
            case CONT_RELATIVE:
                input->type = InputType::Mouse;
                break;
            default:
                input->type = InputType::SingleAnalog;
                break;
        }
    }
}

void initInput()
{
    u8 bitPattern; // TODO use this to repoll
    osContInit(&siMesgQueue, &bitPattern, controllerStatuses);
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        int present = (bitPattern & (1 << i)) != 0;
        set_input_type(&g_InputData[i], controllerStatuses[i].type, present);
        if (present) {
            osMotorInit(&siMesgQueue, &g_Pfs[i], i);
        }
    }
    g_HasEeprom = osEepromProbe(&siMesgQueue);
}

void beginInputPolling()
{
    osContStartQuery(&siMesgQueue);
    osRecvMesg(&siMesgQueue, nullptr, OS_MESG_BLOCK);
    osContGetQuery(controllerStatuses);
    for (int i = 0; i < MAXCONTROLLERS; i++) {
        int present = (controllerStatuses[i].type != 0) && (controllerStatuses[i].errno == 0);
        int wasPresent = g_InputData[i].type == InputType::None;
        set_input_type(&g_InputData[i], controllerStatuses[i].type, present);
        if (present && !wasPresent) {
            osMotorInit(&siMesgQueue, &g_Pfs[i], i);
        }
    }
    osContStartReadData(&siMesgQueue);
}

u8 rumble_states[MAXCONTROLLERS];

void start_rumble(int channel) {
    if (!rumble_states[channel]) {
        osMotorStart(&g_Pfs[channel]);
        rumble_states[channel] = 1;
    }
}

void stop_rumble(int channel) {
    if (rumble_states[channel]) {
        osMotorStop(&g_Pfs[channel]);
        rumble_states[channel] = 0;
    }
}

// Based on N64 & Gamecube controller values
// Maximum x axis value from controller (scaled down a bit to account for controller variation)
#define R0            (72)
#define R0_GCN        (95)
#define R0_GCN_CSTICK (90)
// Angle between x axis and the first octant line segment of the controller bounds octagon (as an s16 angle)
#define ALPHA            (14541) // 79.88 degrees
#define ALPHA_GCN        (12703) // 69.78 degrees
#define ALPHA_GCN_CSTICK (12807) // 70.35 degrees
// Sin of the above alpha value
#define SIN_ALPHA            (0.9844f)
#define SIN_ALPHA_GCN        (0.9384f)
#define SIN_ALPHA_GCN_CSTICK (0.9417f)

#define numControllers 4

extern "C" void bzero(void*, unsigned int);

void readInput()
{
    OSContPad *curPad;
    // OSContPadEx *curPad;
    osRecvMesg(&siMesgQueue, nullptr, OS_MESG_BLOCK);
    osContGetReadData(contPads);
    // osContGetReadDataEx(contPads);
    for (int i = 0; i < numControllers; i++)
    {
        curPad = &contPads[i];
        if (g_InputData[i].type == InputType::SingleAnalog || g_InputData[i].type == InputType::DualAnalog) {
            float x2raw = POW2(curPad->stick_x);
            float y2raw = POW2(curPad->stick_y);
            float curMagnitude = sqrtf(x2raw + y2raw);
            float magnitude;
            float analogScaling;
            s32 angle;
            s32 octantAngle;
            angle = (s32)(u16)atan2s(curPad->stick_x, curPad->stick_y);

            if (angle & 0x2000) // Check if this is in an odd octant
            {
                octantAngle = 0x2000 - (angle & 0x1FFF);
            }
            else
            {
                octantAngle = angle & 0x1FFF;
            }

            if (g_InputData[i].type == InputType::SingleAnalog) {
                analogScaling = R0 * SIN_ALPHA;
                octantAngle += ALPHA;
            } else {
                analogScaling = R0_GCN * SIN_ALPHA_GCN;
                octantAngle += ALPHA_GCN;
            }

            magnitude = MIN(curMagnitude * sinsf(octantAngle) / analogScaling, 1.0f);

            if (magnitude < INPUT_DEADZONE)
            {
                magnitude = 0.0f;
            }

            g_InputData[i].magnitude = magnitude;
            g_InputData[i].angle = angle;
            g_InputData[i].x = (magnitude) * cossf(angle);
            g_InputData[i].y = (magnitude) * sinsf(angle);

            // if (g_InputData[i].type == InputType::DualAnalog) {
            //     float r_x2raw = POW2(curPad->c_stick_x);
            //     float r_y2raw = POW2(curPad->c_stick_y);
            //     float r_curMagnitude = sqrtf(r_x2raw + r_y2raw);
            //     float r_magnitude;
            //     s32 r_angle;
            //     s32 r_octantAngle;
            //     r_angle = (s32)(u16)atan2s(curPad->c_stick_x, curPad->c_stick_y);

            //     if (r_angle & 0x2000) // Check if this is in an odd octant
            //     {
            //         r_octantAngle = 0x2000 - (r_angle & 0x1FFF);
            //     }
            //     else
            //     {
            //         r_octantAngle = r_angle & 0x1FFF;
            //     }

            //     r_magnitude = MIN(r_curMagnitude * sinsf(r_octantAngle + ALPHA_GCN_CSTICK) / (R0_GCN_CSTICK * SIN_ALPHA_GCN_CSTICK), 1.0f);

            //     if (r_magnitude < INPUT_DEADZONE)
            //     {
            //         r_magnitude = 0.0f;
            //     }
            //     g_InputData[i].r_x = (r_magnitude) * cossf(r_angle);
            //     g_InputData[i].r_y = (r_magnitude) * sinsf(r_angle);
            //     g_InputData[i].l_trig = curPad->l_trig / 255.0f;
            //     g_InputData[i].r_trig = curPad->r_trig / 255.0f;
            // }
        } else {
            g_InputData[i].x = curPad->stick_x / 128.0f;
            g_InputData[i].y = curPad->stick_y / 128.0f;
        }
        g_InputData[i].buttonsPressed = (curPad->button) & ~g_InputData[i].buttonsHeld;
        g_InputData[i].buttonsHeld = curPad->button;
    }
}
