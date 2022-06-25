// libultra
#include <ultra64.h>

// game code
extern "C" {
#include <debug.h>
}
#include <main.h>
#include <gfx.h>
#include <mem.h>
#include <audio.h>
#include <n64_task_sched.h>
#include <n64_init.h>
#include <n64_gfx.h>
#include <n64_mem.h>
#include <n64_audio.h>
#include <title.h>
#include <text.h>
#include <input.h>

extern "C" void bzero(void*, unsigned int);

void loadThreadFunc(void *);

#include <array>

void platformInit()
{
}

u8 idleThreadStack[IDLE_THREAD_STACKSIZE] __attribute__((aligned (16)));
u8 mainThreadStack[MAIN_THREAD_STACKSIZE] __attribute__((aligned (16)));
u8 audioThreadStack[AUDIO_THREAD_STACKSIZE] __attribute__((aligned (16)));
u8 loadThreadStack[LOAD_THREAD_STACKSIZE] __attribute__((aligned (16)));

static OSMesgQueue piMesgQueue;
static std::array<OSMesg, NUM_PI_MESSAGES> piMessages;

OSMesgQueue siMesgQueue;
static std::array<OSMesg, NUM_SI_MESSAGES> siMessages;

#define _MIPS_SIM_ABI32  1
#define _MIPS_SIM_NABI32 2
#define _MIPS_SIM_ABI64  3

typedef struct {
    u64 at, v0, v1, a0, a1, a2, a3;
    u64 t0, t1, t2, t3, t4, t5, t6, t7;
    u64 s0, s1, s2, s3, s4, s5, s6, s7;
    u64 t8, t9;
    u64 gp, sp, s8, ra;
    u64 lo, hi;
    u32 sr, pc, cause, badvaddr, rcp;
    u32 fpcsr;
    __OSfp  fp0,  fp2,  fp4,  fp6,  fp8, fp10, fp12, fp14;
    __OSfp fp16, fp18, fp20, fp22, fp24, fp26, fp28, fp30;
#if (_MIPS_SIM == _MIPS_SIM_ABI64) || (_MIPS_SIM == _MIPS_SIM_NABI32)
    __OSfp  fp1,  fp3,  fp5,  fp7,  fp9, fp11, fp13, fp15;
    __OSfp fp17, fp19, fp21, fp23, fp25, fp27, fp29, fp31;
#endif
} __OSThreadContext2;

typedef struct {
    u32 flag;
    u32 count;
    u64 time;
} __OSThreadprofile_s2;

typedef struct OSThread_s2 {
    struct OSThread_s2   *next;       /* run/mesg queue link */
    OSPri                 priority;   /* run/mesg queue priority */
    struct OSThread_s2  **queue;      /* queue thread is on */
    struct OSThread_s2   *tlnext;     /* all threads queue link */
    u16                   state;      /* OS_STATE_* */
    u16                   flags;      /* flags for rmon */
    OSId                  id;         /* id for debugging */
    int                   fp;         /* thread has used fp unit */
    __OSThreadprofile_s2 *thprof;     /* workarea for thread profiler */
    __OSThreadContext2    context;    /* register/interrupt mask */
} OSThread2;

static std::array<OSThread2, NUM_THREADS> g_threads;
OSPiHandle *g_romHandle;

u8 g_isEmulator;

extern "C" void init(void)
{
    bzero(_mainSegmentBssStart, (u32)_mainSegmentBssEnd - (u32)_mainSegmentBssStart);
    osInitialize();
    
    if (IO_READ(DPC_CLOCK_REG) == 0) {
        g_isEmulator = 1;
    }

    // TODO figure out what's uninitialized that requires this to work
    // bzero(_mainSegmentBssEnd, MEM_END -  (u32)_mainSegmentBssEnd);

    initMemAllocator(memPoolStart, (void*) MEM_END);
    g_romHandle = osCartRomInit();

    osCreateThread((OSThread*)&g_threads[IDLE_THREAD_INDEX], IDLE_THREAD, idle, nullptr, idleThreadStack + IDLE_THREAD_STACKSIZE, 10);
    osStartThread((OSThread*)&g_threads[IDLE_THREAD_INDEX]);
}

int main(int, char **);
extern "C" u32 __osSetFpcCsr(u32);

// void crash_screen_init();

void mainThreadFunc(void *)
{
    u32 fpccsr;

    // Read fpcsr
    fpccsr = __osSetFpcCsr(0);
    // Write back fpcsr with division by zero and invalid operations exceptions disabled
    __osSetFpcCsr(fpccsr & ~(FPCSR_EZ | FPCSR_EV));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    main(0, nullptr);
#pragma GCC diagnostic pop
}

void idle(__attribute__ ((unused)) void *arg)
{
    bzero(g_frameBuffers.data(), num_frame_buffers * screen_width * screen_height * sizeof(u16));

    initScheduler();
    
    // Set up PI
    osCreatePiManager(OS_PRIORITY_PIMGR, &piMesgQueue, piMessages.data(), NUM_PI_MESSAGES);

    // Set up SI
    osCreateMesgQueue(&siMesgQueue, siMessages.data(), NUM_SI_MESSAGES);
    osSetEventMesg(OS_EVENT_SI, &siMesgQueue, nullptr);

#ifdef DEBUG_MODE
    debug_initialize();
#endif

    // // Create the audio thread
    // osCreateThread(&g_threads[AUDIO_THREAD_INDEX], AUDIO_THREAD, audioThreadFunc, nullptr, audioThreadStack + AUDIO_THREAD_STACKSIZE, AUDIO_THREAD_PRI);
    // // Start the audio thread
    // osStartThread(&g_threads[AUDIO_THREAD_INDEX]);

    // Create the load thread
    osCreateThread((OSThread*)&g_threads[LOAD_THREAD_INDEX], LOAD_THREAD, loadThreadFunc, nullptr, loadThreadStack + LOAD_THREAD_STACKSIZE, LOAD_THREAD_PRI);
    // Start the load thread
    osStartThread((OSThread*)&g_threads[LOAD_THREAD_INDEX]);

    // Create the main thread
    osCreateThread((OSThread*)&g_threads[MAIN_THREAD_INDEX], MAIN_THREAD, mainThreadFunc, nullptr, mainThreadStack + MAIN_THREAD_STACKSIZE, MAIN_THREAD_PRI);
    // Start the main thread
    osStartThread((OSThread*)&g_threads[MAIN_THREAD_INDEX]);

    // crash_screen_init();

    // Set this thread's priority to 0, making it the idle thread
    osSetThreadPri(nullptr, 0);

    // idle
    while (1);
}

struct TileAxisState {
    int low;
    int high;
    int shift;
    int mask;
    int mirror;
    int clamp;
    int scale;
};

struct TileState {
    TileAxisState s_state;
    TileAxisState t_state;
    uint16_t s_scale;
    uint16_t t_scale;
};

extern Gfx* g_gui_dlist_head;

#define ORTHO

#ifdef ORTHO
constexpr int z = 0;
#else
constexpr int z = 20;
#endif

Vtx verts[] = {
    {{{-64, -64 - 40, -z}, 0, {-4096,-4096}, {0, 0, 0, 0}}},
    {{{ 64, -64 - 40,  z}, 0, { 4096,-4096}, {0, 0, 0, 0}}},
    {{{-64,  64 - 40, -z}, 0, {-4096, 4096}, {0, 0, 0, 0}}},
    {{{ 64,  64 - 40,  z}, 0, { 4096, 4096}, {0, 0, 0, 0}}},
};

Mtx ident_matrix = float_to_fixed({
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f},
});

int calculate_cm(const TileAxisState& state) {
    int ret = 0;

    if (state.clamp) {
        ret |= G_TX_CLAMP;
    } else {
        ret |= G_TX_WRAP;
    }

    if (state.mirror) {
        ret |= G_TX_MIRROR;
    } else {
        ret |= G_TX_NOMIRROR;
    }

    return ret;
}

Gfx* load_texture(Gfx* dl_head, int index, void* tex_data, const TileState& state) {
    int cms = calculate_cm(state.s_state);
    int cmt = calculate_cm(state.t_state);

    // if (index == 0) {
    //     gDPLoadMultiBlock(dl_head++, tex_data, (2048 / sizeof(uint64_t)) * index, index, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0,
    //         cms, cmt,
    //         state.s_state.mask, state.t_state.mask,
    //         state.s_state.shift, state.t_state.shift);
    //     gDPSetTileSize(dl_head++, index, state.s_state.low, state.t_state.low, state.s_state.high, state.t_state.high);
    // } else {
    //     gDPLoadMultiBlock(dl_head++, tex_data, (2048 / sizeof(uint64_t)) * index, index, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
    //         cms, cmt,
    //         state.s_state.mask, state.t_state.mask,
    //         state.s_state.shift, state.t_state.shift);
    //     gDPSetTileSize(dl_head++, index, state.s_state.low, state.t_state.low, state.s_state.high, state.t_state.high);
    // }
    
    gDPLoadMultiBlock(dl_head++, tex_data, (2048 / sizeof(uint64_t)) * index, index, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
        cms, cmt,
        state.s_state.mask, state.t_state.mask,
        state.s_state.shift, state.t_state.shift);
    gDPSetTileSize(dl_head++, index, state.s_state.low, state.t_state.low, state.s_state.high, state.t_state.high);

    return dl_head;
}

void pick_text_color(int selected_column, int column, int selected_field, int row) {
    if (selected_column == column && selected_field == row) {
        set_text_color(0, 255, 0, 255);
    } else {
        set_text_color(255, 255, 255, 255);
    }
}

void print_tile_state(const TileState& state, int index, int x, int y, int selected_column, int selected_field) {
    char buf[256];
    int row = 0;
    int left_x = x;
    int right_x = 6 * 7 + x;

    sprintf(buf, "TILE%d\n", index);
    print_text(x, y +  0, buf);
    print_text(x, y + 10, "S     T\n");

    y += 20;

    sprintf(buf, "%-6d", state.s_state.clamp);
    pick_text_color(selected_column, 0, selected_field, row);
    print_text(left_x, y, buf);
    sprintf(buf, "%-6d", state.t_state.clamp);
    pick_text_color(selected_column, 1, selected_field, row++);
    print_text(right_x, y, buf);

    y += 10;

    sprintf(buf, "%-6d", state.s_state.mirror);
    pick_text_color(selected_column, 0, selected_field, row);
    print_text(left_x, y, buf);
    sprintf(buf, "%-6d", state.t_state.mirror);
    pick_text_color(selected_column, 1, selected_field, row++);
    print_text(right_x, y, buf);

    y += 10;

    sprintf(buf, "%-6d", state.s_state.shift);
    pick_text_color(selected_column, 0, selected_field, row);
    print_text(left_x, y, buf);
    sprintf(buf, "%-6d", state.t_state.shift);
    pick_text_color(selected_column, 1, selected_field, row++);
    print_text(right_x, y, buf);

    y += 10;

    sprintf(buf, "%-6.2f", (double)(state.s_state.low  / 4.0f));
    pick_text_color(selected_column, 0, selected_field, row);
    print_text(left_x, y, buf);
    sprintf(buf, "%-6.2f", (double)(state.t_state.low  / 4.0f));
    pick_text_color(selected_column, 1, selected_field, row++);
    print_text(right_x, y, buf);

    y += 10;

    sprintf(buf, "%-6.2f", (double)(state.s_state.high / 4.0f));
    pick_text_color(selected_column, 0, selected_field, row);
    print_text(left_x, y, buf);
    sprintf(buf, "%-6.2f", (double)(state.t_state.high  / 4.0f));
    pick_text_color(selected_column, 1, selected_field, row++);
    print_text(right_x, y, buf);

    y += 10;

    sprintf(buf, "%-6d", state.s_state.mask);
    pick_text_color(selected_column, 0, selected_field, row);
    print_text(left_x, y, buf);
    sprintf(buf, "%-6d", state.t_state.mask);
    pick_text_color(selected_column, 1, selected_field, row++);
    print_text(right_x, y, buf);

    // Draw texture scale, but only for index 0 since it affects both tiles
    if (index == 0) {
        y -= 100;
        row = -2;

        sprintf(buf, "0X%04X", state.s_scale);
        pick_text_color(selected_column, 0, selected_field, row++);
        print_text(left_x, y, buf);

        y += 10;

        sprintf(buf, "0X%04X", state.t_scale);
        pick_text_color(selected_column, 0, selected_field, row++);
        print_text(left_x, y, buf);
    }
}

template <typename T>
void clamp(T& val, int min, int max) {
    val = static_cast<T>(MIN(MAX(val, min), max));
}

template <typename T>
void wrap(T& val, int min, int max) {
    if (static_cast<int>(val) < min) {
        val = static_cast<T>(static_cast<int>(val) + max - min);
    }
    if (static_cast<int>(val) >= max) {
        val = static_cast<T>(static_cast<int>(val) - max + min);
    }
}

void modify_value(TileAxisState& state, int field, int delta) {
    switch (field) {
        case 0: // clamp
            clamp(delta, -1, 1);
            state.clamp += delta;
            wrap(state.clamp, 0, 2);
            break;
        case 1: // mirror
            clamp(delta, -1, 1);
            state.mirror += delta;
            wrap(state.mirror, 0, 2);
            break;
        case 2: // shift
            clamp(delta, -1, 1);
            state.shift += delta;
            wrap(state.shift, 0, 16);
            break;
        case 3: // low
            state.low += delta;
            wrap(state.low, 0, 1024 * 4);
            break;
        case 4: // high
            state.high += delta;
            wrap(state.high, 0, 1024 * 4);
            break;
        case 5: // mask
            clamp(delta, -1, 1);
            state.mask += delta;
            wrap(state.mask, 0, 7);
            break;
    }
}

consteval TileAxisState init_tile_axis_state() {
    TileAxisState ret{};

    ret.mask = 5;
    ret.high = 31 << 2;

    return ret;
}

consteval TileState init_tile_state() {
    return TileState{init_tile_axis_state(), init_tile_axis_state(), 0x8000, 0x8000};
}

void TitleScene::draw(UNUSED bool unloading) {
    static int selected_column = 0;
    static int selected_field = 0;
    static bool two_tiles = false;
    static bool use_texrects = false;
    static bool use_bilerp = false;
    static TileState tile_states[2] = {init_tile_state(), init_tile_state()};

    if (g_InputData[0].buttonsPressed & R_JPAD) {
        selected_column++;
    }
    if (g_InputData[0].buttonsPressed & L_JPAD) {
        selected_column--;
    }

    if (selected_column < 0) {
        selected_column = 0;
    }

    if (selected_column > 3) {
        selected_column = 3;
    }

    if (g_InputData[0].buttonsPressed & D_JPAD) {
        selected_field++;
    }
    if (g_InputData[0].buttonsPressed & U_JPAD) {
        selected_field--;
    }

    if (selected_field < -2) {
        selected_field = -2;
    }

    if (selected_field > 5) {
        selected_field = 5;
    }

    // Force S/T scale to be column 0
    if (selected_field < 0) {
        selected_column = 0;
    }

    int delta = 0;

    if ((g_InputData[0].buttonsPressed & U_CBUTTONS) || (g_InputData[0].buttonsPressed & A_BUTTON)) {
        delta++;
    }

    if ((g_InputData[0].buttonsPressed & D_CBUTTONS) || (g_InputData[0].buttonsPressed & B_BUTTON)) {
        delta--;
    }

    if (g_InputData[0].buttonsHeld & R_TRIG) {
        delta *= 8;
    }

    if (g_InputData[0].buttonsPressed & START_BUTTON) {
        two_tiles ^= 1;
    }

    if (g_InputData[0].buttonsPressed & L_TRIG) {
        use_texrects ^= 1;
    }

    if (g_InputData[0].buttonsPressed & Z_TRIG) {
        use_bilerp ^= 1;
    }

    if (delta != 0) {
        // Everything except S/T scale
        if (selected_field > 0) {
            switch (selected_column % 2) {
                case 0:
                    modify_value(tile_states[selected_column / 2].s_state, selected_field, delta);
                    break;
                case 1:
                    modify_value(tile_states[selected_column / 2].t_state, selected_field, delta);
                    break;
            }
        } else {
            switch (selected_field) {
                case -2: // S scale
                    delta *= 0x100;
                    tile_states[0].s_scale += delta;
                    break;
                case -1: // T scale
                    delta *= 0x100;
                    tile_states[0].t_scale += delta;
                    break;
            }
        }
    }

    Gfx* dl_head = g_gui_dlist_head;
    Mtx* proj_matrix = (Mtx*)allocGfx(sizeof(Mtx));

    u16 perspNorm = 0xFFFF;
#ifdef ORTHO
    guOrtho(proj_matrix, -160.0f, 160.0f, 120.0f, -120.0f, -1.0f, 1.0f, 4.0f);
#else
    MtxF proj_matrix_f;
    MtxF persp_matrix_f;
    MtxF lookat_matrix_f;
    guPerspectiveF(persp_matrix_f, &perspNorm, 45.0f, 4.0f / 3.0f, 0.1f, 1000.0f, 1.0f);
    guLookAtF(lookat_matrix_f, 0.0f, 0.0f, -288.8f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f);
    guMtxCatF(lookat_matrix_f, persp_matrix_f, proj_matrix_f);
    guMtxF2L(proj_matrix_f, proj_matrix);
#endif

    gDPPipeSync(dl_head++);

    dl_head = load_texture(dl_head, 0, tex0_, tile_states[0]);
    dl_head = load_texture(dl_head, 1, tex1_, tile_states[1]);

    gDPSetRenderMode(dl_head++, G_RM_PASS, G_RM_OPA_SURF2);
    gDPSetCycleType(dl_head++, G_CYC_2CYCLE);

    if (use_bilerp) {
        gDPSetTextureFilter(dl_head++, G_TF_BILERP);
    } else {
        gDPSetTextureFilter(dl_head++, G_TF_POINT);
    }

    if (two_tiles) {
        gDPSetCombineLERP(dl_head++, TEXEL1, TEXEL0, TEXEL1_ALPHA, TEXEL0,  0, 0, 0, 0,  0, 0, 0, COMBINED,  0, 0, 0, 1);
    } else {
        gDPSetCombineLERP(dl_head++, 0, 0, 0, TEXEL0,  0, 0, 0, 0,  0, 0, 0, COMBINED,  0, 0, 0, 1);
    }

    // if (two_tiles) {
    //     gDPSetCombineLERP(dl_head++, 0, 0, 0, TEXEL0,  0, 0, 0, 0,  0, 0, 0, TEXEL0,  0, 0, 0, 1);
    // } else {
    //     gDPSetCombineLERP(dl_head++, 0, 0, 0, TEXEL0,  0, 0, 0, 0,  0, 0, 0, COMBINED,  0, 0, 0, 1);
    // }

    gSPViewport(dl_head++, &gfx::viewport);
    gSPLoadGeometryMode(dl_head++, 0);
    gSPPerspNormalize(g_dlist_head++, perspNorm);
    gSPTexture(dl_head++, tile_states[0].s_scale, tile_states[0].t_scale, 0, 0, G_ON);
    gSPMatrix(dl_head++, proj_matrix, G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPMatrix(dl_head++, &ident_matrix, G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    // gDPSetCombineLERP(dl_head++, 0, 0, 0, TEXEL0,  0, 0, 0, 0,  0, 0, 0, COMBINED,  0, 0, 0, 1);
    // gDPSetCombineLERP(dl_head++, 0, 0, 0, 1,  0, 0, 0, 0,  0, 0, 0, COMBINED,  0, 0, 0, 1);

    if (use_texrects) {
        gDPSetTexturePersp(dl_head++, G_TP_NONE);
        gSPTextureRectangle(dl_head++,
            (160 - 64) << 2, (120 - 64 - 40) << 2,
            (160 + 64) << 2, (120 + 64 - 40) << 2,
            0,
            -64 * 32, -64 * 32,
             1 << 10, 1 << 10);
    } else {
        gDPSetTexturePersp(dl_head++, G_TP_PERSP);
        gSPVertex(dl_head++, &verts, sizeof(verts) / sizeof(verts[0]), 0);
        gSP2Triangles(dl_head++, 0, 1, 2, 0x00,  2, 1, 3, 0x00);
    }

    set_text_color(255, 255, 255, 255);

    constexpr int text_y = 150;
    constexpr int text_x = 10;

    print_text(text_x, text_y - 30, 
        "SCALES\n"
        "SCALET\n"
        "\n"
        "\n"
        "\n"
        "CLAMP\n"
        "MIRROR\n"
        "SHIFT\n"
        "LOW\n"
        "HIGH\n"
        "MASK"
    );

    if (!two_tiles) {
        clamp(selected_column, 0, 1);
    }

    set_text_color(255, 255, 255, 255);
    print_tile_state(tile_states[0], 0, text_x + 40, text_y, selected_column % 2, selected_column < 2 ? selected_field : -1);

    if (two_tiles) {
        set_text_color(255, 255, 255, 255);
        print_tile_state(tile_states[1], 1, text_x + 126, text_y, selected_column % 2, selected_column >= 2 ? selected_field : -1);
    }

    set_text_color(255, 255, 255, 255);

    print_text(text_x + 206, text_y,      "START TO TOGGLE");
    print_text(text_x + 206, text_y + 10, two_tiles ? "    2 TILES" : "    1 TILE");
    
    print_text(text_x + 206 + 2 * 6, text_y + 30, "L TO TOGGLE");
    print_text(text_x + 206 + 2 * 6, text_y + 40, use_texrects ? "  TEXRECT" : " TRIANGLES");

    print_text(text_x + 206 + 2 * 6, text_y + 60, "Z TO TOGGLE");
    print_text(text_x + 206 + 2 * 6, text_y + 70, use_bilerp ? "  BILERP" : "   POINT");

    g_gui_dlist_head = dl_head;

    draw_all_text();
}
