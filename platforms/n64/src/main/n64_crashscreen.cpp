#include <cstdarg>
#include <cstring>

#include <ultra64.h>
#include <PR/os_internal_error.h>

#include <types.h>
#include <input.h>
#include <n64_gfx.h>
#include <n64_init.h>
#include <n64_mem.h>
#include <vassert.h>

char assertion_string_buf[128];
const char* assertion_exception;
const char* assertion_filename;
int assertion_linenum;
uintptr_t assertion_ra;

extern "C" int sprintf(char *s, const char *fmt, ...);

uint8_t gCrashScreenCharToGlyph[128] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF,   41, 0xFF, 0xFF, 0xFF,   43, 0xFF, 0xFF,   37,   38, 0xFF,   42, 0xFF,   39,   44,   46,
       0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   36, 0xFF, 0xFF,   45,   48,   40,
    0xFF,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
      25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35, 0xFF, 0xFF, 0xFF, 0xFF,   47,
    0xFF,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
      25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

uint32_t gCrashScreenFont[7 * 10 + 1] = {
    #include "crash_screen_font.ia1.inc"
};

const char *gCauseDesc[18] = {
    "Interrupt",
    "TLB modification",
    "TLB exception on load",
    "TLB exception on store",
    "Address error on load",
    "Address error on store",
    "Bus error on inst.",
    "Bus error on data",
    "System call exception",
    "Breakpoint exception",
    "Reserved instruction",
    "Coprocessor unusable",
    "Arithmetic overflow",
    "Trap exception",
    "Virtual coherency on inst.",
    "Floating point exception",
    "Watchpoint exception",
    "Virtual coherency on data",
};

const char *gFpcsrDesc[6] = {
    "Unimplemented operation", "Invalid operation", "Division by zero", "Overflow", "Underflow",
    "Inexact operation",
};



extern uint64_t osClockRate;

struct {
    OSThread thread;
    uint64_t stack[0x800 / sizeof(uint64_t)];
    OSMesgQueue mesgQueue;
    OSMesg mesg;
    uint16_t *framebuffer;
    uint16_t width;
    uint16_t height;
} gCrashScreen;

void crash_screen_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
    uint16_t *ptr;
    int32_t i, j;

    ptr = gCrashScreen.framebuffer + gCrashScreen.width * y + x;
    for (i = 0; i < h; i++) {
        for (j = 0; j < w; j++) {
            // 0xe738 = 0b1110011100111000
            // *ptr = ((*ptr & 0xe738) >> 2) | 1;
            *ptr = 1;
            ptr++;
        }
        ptr += gCrashScreen.width - w;
    }
}

void crash_screen_draw_glyph(int32_t x, int32_t y, int32_t glyph) {
    const uint32_t *data;
    uint16_t *ptr;
    uint32_t bit;
    uint32_t rowMask;
    int32_t i, j;

    data = &gCrashScreenFont[glyph / 5 * 7];
    ptr = gCrashScreen.framebuffer + gCrashScreen.width * y + x;

    for (i = 0; i < 7; i++) {
        bit = 0x80000000U >> ((glyph % 5) * 6);
        rowMask = *data++;

        for (j = 0; j < 6; j++) {
            *ptr++ = (bit & rowMask) ? 0xffff : 1;
            bit >>= 1;
        }
        ptr += gCrashScreen.width - 6;
    }
}

void crash_screen_draw_string(int32_t x, int32_t y, const char *buf) {
    uint32_t glyph;
    int cur_char;

    int32_t start_x = x;

    while ((cur_char = *buf) != '\x00') {
        if (cur_char == '\n') {
            x = start_x;
            y += 10;
        } else {
            glyph = gCrashScreenCharToGlyph[*buf & 0x7f];

            if (glyph != 0xff) {
                crash_screen_draw_glyph(x, y, glyph);
            }

            x += 6;
        }
        buf++;
    }
}

template <typename... Ts>
void crash_screen_print(int32_t x, int32_t y, const char *fmt, Ts... args)
{
    char buf[100];
    sprintf(buf, fmt, args...);
    crash_screen_draw_string(x, y, buf);
}

extern "C" void crash_screen_sleep(int32_t ms) {
    uint64_t cycles = ms * 1000LL * osClockRate / 1000000ULL;
    osSetTime(0);
    while (osGetTime() < cycles) {
    }
}

void crash_screen_print_float_reg(int32_t x, int32_t y, int32_t regNum, void *addr) {
    uint32_t bits;
    int32_t exponent;

    bits = *(uint32_t *) addr;
    exponent = ((bits & 0x7f800000U) >> 0x17) - 0x7f;
    if ((exponent >= -0x7e && exponent <= 0x7f) || bits == 0) {
        crash_screen_print(x, y, "F%02d:%.3e", regNum, static_cast<double>(*(float *) addr));
    } else {
        crash_screen_print(x, y, "F%02d:---------", regNum);
    }
}

void crash_screen_print_fpcsr(uint32_t fpcsr) {
    int32_t i;
    uint32_t bit;

    bit = 1 << 17;
    crash_screen_print(30, 155, "FPCSR:%08XH", fpcsr);
    for (i = 0; i < 6; i++) {
        if (fpcsr & bit) {
            crash_screen_print(132, 155, "(%s)", gFpcsrDesc[i]);
            return;
        }
        bit >>= 1;
    }
}

void draw_assertion_screen() {

    osInvalDCache(gCrashScreen.framebuffer, gCrashScreen.width * gCrashScreen.height * sizeof(u16));
    crash_screen_draw_rect(25, 20, 270, 25 + 185);
    crash_screen_print(30, 25, "ASSERTION FAILED: %s", assertion_exception);
    crash_screen_print(30, 35, "  FILE: %s", assertion_filename);
    crash_screen_print(30, 45, "  LINE: %d", assertion_linenum);
    crash_screen_print(30, 55, "RETURN ADDR: %08X", (uint32_t)assertion_ra);
    crash_screen_print(30, 65, assertion_string_buf);

    osWritebackDCacheAll();
    osViBlack(FALSE);
    osViSwapBuffer(gCrashScreen.framebuffer);
}

void draw_crash_screen(OSThread *thread) {
    s16 cause;
    __OSThreadContext *tc = &thread->context;

    cause = (tc->cause >> 2) & 0x1f;
    if (cause == 23) // EXC_WATCH
    {
        cause = 16;
    }
    if (cause == 31) // EXC_VCED
    {
        cause = 17;
    }

    crash_screen_draw_rect(25, 20, 270, 25);
    crash_screen_print(30, 25, "THREAD:%d  (%s)", thread->id, gCauseDesc[cause]);
    crash_screen_print(30, 35, "PC:%08x    SR:%08x    VA:%08x", tc->pc, tc->sr, tc->badvaddr);
    crash_screen_draw_rect(25, 45, 270, 185);
    crash_screen_print(30, 50, "AT:%08XH   V0:%08XH   V1:%08XH", (uint32_t) tc->at, (uint32_t) tc->v0,
                       (uint32_t) tc->v1);
    crash_screen_print(30, 60, "A0:%08XH   A1:%08XH   A2:%08XH", (uint32_t) tc->a0, (uint32_t) tc->a1,
                       (uint32_t) tc->a2);
    crash_screen_print(30, 70, "A3:%08XH   T0:%08XH   T1:%08XH", (uint32_t) tc->a3, (uint32_t) tc->t0,
                       (uint32_t) tc->t1);
    crash_screen_print(30, 80, "T2:%08XH   T3:%08XH   T4:%08XH", (uint32_t) tc->t2, (uint32_t) tc->t3,
                       (uint32_t) tc->t4);
    crash_screen_print(30, 90, "T5:%08XH   T6:%08XH   T7:%08XH", (uint32_t) tc->t5, (uint32_t) tc->t6,
                       (uint32_t) tc->t7);
    crash_screen_print(30, 100, "S0:%08XH   S1:%08XH   S2:%08XH", (uint32_t) tc->s0, (uint32_t) tc->s1,
                       (uint32_t) tc->s2);
    crash_screen_print(30, 110, "S3:%08XH   S4:%08XH   S5:%08XH", (uint32_t) tc->s3, (uint32_t) tc->s4,
                       (uint32_t) tc->s5);
    crash_screen_print(30, 120, "S6:%08XH   S7:%08XH   T8:%08XH", (uint32_t) tc->s6, (uint32_t) tc->s7,
                       (uint32_t) tc->t8);
    crash_screen_print(30, 130, "T9:%08XH   GP:%08XH   SP:%08XH", (uint32_t) tc->t9, (uint32_t) tc->gp,
                       (uint32_t) tc->sp);
    crash_screen_print(30, 140, "S8:%08XH   RA:%08XH", (uint32_t) tc->s8, (uint32_t) tc->ra);
    crash_screen_print_fpcsr(tc->fpcsr);
    osWritebackDCacheAll();
    crash_screen_print_float_reg(30, 170, 0, &tc->fp0.f.f_even);
    crash_screen_print_float_reg(120, 170, 2, &tc->fp2.f.f_even);
    crash_screen_print_float_reg(210, 170, 4, &tc->fp4.f.f_even);
    crash_screen_print_float_reg(30, 180, 6, &tc->fp6.f.f_even);
    crash_screen_print_float_reg(120, 180, 8, &tc->fp8.f.f_even);
    crash_screen_print_float_reg(210, 180, 10, &tc->fp10.f.f_even);
    crash_screen_print_float_reg(30, 190, 12, &tc->fp12.f.f_even);
    crash_screen_print_float_reg(120, 190, 14, &tc->fp14.f.f_even);
    crash_screen_print_float_reg(210, 190, 16, &tc->fp16.f.f_even);
    crash_screen_print_float_reg(30, 200, 18, &tc->fp18.f.f_even);
    crash_screen_print_float_reg(120, 200, 20, &tc->fp20.f.f_even);
    crash_screen_print_float_reg(210, 200, 22, &tc->fp22.f.f_even);
    crash_screen_print_float_reg(30, 210, 24, &tc->fp24.f.f_even);
    crash_screen_print_float_reg(120, 210, 26, &tc->fp26.f.f_even);
    crash_screen_print_float_reg(210, 210, 28, &tc->fp28.f.f_even);
    crash_screen_print_float_reg(30, 220, 30, &tc->fp30.f.f_even);
    osWritebackDCacheAll();
    osViBlack(FALSE);
    osViSwapBuffer(gCrashScreen.framebuffer);
}

OSThread *get_crashed_thread(void) {
    OSThread *thread;

    thread = __osGetCurrFaultedThread();
    while (thread->priority != -1) {
        if (thread->priority > OS_PRIORITY_IDLE && thread->priority < OS_PRIORITY_APPMAX
            && (thread->flags & 3) != 0) {
            return thread;
        }
        thread = thread->tlnext;
    }
    return NULL;
}

void fault_detection_thread(UNUSED void *arg) {
    OSMesg mesg;
    OSThread *thread;

    osSetEventMesg(OS_EVENT_CPU_BREAK, &gCrashScreen.mesgQueue, (OSMesg) 1);
    osSetEventMesg(OS_EVENT_FAULT, &gCrashScreen.mesgQueue, (OSMesg) 2);
    do {
        osRecvMesg(&gCrashScreen.mesgQueue, &mesg, 1);
        thread = get_crashed_thread();
    } while (thread == NULL);
    gCrashScreen.framebuffer = g_frameBuffers[g_curGfxContext ^ 1].data();

    u32 start = osGetCount();
    while (osGetCount() - start < OS_USEC_TO_CYCLES(1000000)) {
        // spin long enough to ensure RDP is done before drawing to framebuffer
    }

    if (assertion_filename == nullptr) {
        draw_crash_screen(thread);
    } else {
        draw_assertion_screen();
    }
    for (;;) {
        gCrashScreen.framebuffer = g_frameBuffers[g_curGfxContext].data();
        g_curGfxContext ^= 1;
        draw_assertion_screen();
        beginInputPolling();
        osRecvMesg(&siMesgQueue, nullptr, OS_MESG_BLOCK);
        start = osGetCount();
        while (osGetCount() - start < OS_USEC_TO_CYCLES(10000)) {
        }
    }
}

void crash_screen_set_framebuffer(uint16_t *framebuffer, uint16_t width, uint16_t height) {
    gCrashScreen.framebuffer = framebuffer;
    gCrashScreen.width = width;
    gCrashScreen.height = height;
}

void crash_screen_init(void) {
    gCrashScreen.width = screen_width;
    gCrashScreen.height = screen_height;
    osCreateMesgQueue(&gCrashScreen.mesgQueue, &gCrashScreen.mesg, 1);
    osCreateThread(&gCrashScreen.thread, CRASH_THREAD, fault_detection_thread, NULL,
                   (uint8_t *) gCrashScreen.stack + sizeof(gCrashScreen.stack),
                   CRASH_THREAD_PRI
                  );
    osStartThread(&gCrashScreen.thread);
}

typedef char *outfun(char*,const char*,size_t);
extern "C" int _Printf(outfun prout, char *arg, const char *fmt, va_list args);
static char *proutSprintf(char *dst, const char *src, size_t count);

void vassert_impl(const char* exception, const char* filename, int linenum, void* ra, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    assertion_exception = exception;
    assertion_filename = filename;
    assertion_linenum = linenum;
    assertion_ra = (uintptr_t)ra;

    _Printf(proutSprintf, assertion_string_buf, fmt, args);

    *(volatile int*)0 = 0;
}

static char *proutSprintf(char *dst, const char *src, size_t count)
{
    return (char *)memcpy((u8 *)dst, (u8 *)src, count) + count;
}
