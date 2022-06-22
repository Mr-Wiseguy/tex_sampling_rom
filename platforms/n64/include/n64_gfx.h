#ifndef __N64_GFX_H__
#define __N64_GFX_H__

#include <config.h>
#include <gfx.h>
#include <PR/gbi.h>
#include <PR/os_thread.h>
#include <PR/os_message.h>

// Fix for gSPLoadUcodeL on gcc
#ifdef gSPLoadUcodeL
#undef gSPLoadUcodeL
#endif

#define	gSPLoadUcodeL(pkt, ucode)					\
        gSPLoadUcode((pkt), OS_K0_TO_PHYSICAL(&ucode##TextStart),	\
		            OS_K0_TO_PHYSICAL(&ucode##DataStart))

constexpr unsigned int output_buff_len = 1024;

constexpr unsigned int display_list_len = 1024;
constexpr unsigned int gui_display_list_len = 4096;
constexpr unsigned int gfx_pool_size = 65536 * 8;
constexpr unsigned int gfx_pool_size64 = gfx_pool_size / 8;

constexpr unsigned int num_frame_buffers = 2;

#ifdef HIGH_RES
constexpr int screen_width = 640;
constexpr int screen_height = 480;
#else
constexpr int screen_width = 320;
constexpr int screen_height = 240;
constexpr int border_height = 8;
#endif

#define BUFFER_SEGMENT 0x01

#ifdef OS_K0_TO_PHYSICAL
 #undef OS_K0_TO_PHYSICAL
#endif

// Gets rid of warnings with -Warray-bounds
#define OS_K0_TO_PHYSICAL(x) ((u32)(x)-0x80000000)

struct GfxContext {
    // Master displaylist
    Gfx dlist_buffer[display_list_len];
    // Gui displaylist
    Gfx gui_dlist_buffer[gui_display_list_len];
    // Floating point model matrix stack
    MtxF mtxFStack[matf_stack_len];
    // Floating point projection matrix
    MtxF projMtxF;
    // Floating point view matrix
    MtxF viewMtxF;
    // Floating point view*proj matrix
    MtxF viewProjMtxF;
    // Graphics tasks done message
    OSMesg taskDoneMesg;
    // Graphics tasks done message queue
    OSMesgQueue taskDoneQueue;
    // Graphics pool
    u64 pool[gfx_pool_size64];
};

extern struct GfxContext g_gfxContexts[num_frame_buffers];

extern Mtx *g_curMatPtr;
extern u32 g_curGfxContext;
extern u16 g_perspNorm;
extern Gfx *g_dlist_head;

#include <array>
extern std::array<std::array<u16, screen_width * screen_height>, num_frame_buffers> g_frameBuffers;

void addGfxToDrawLayer(DrawLayer drawLayer, Gfx* toAdd);
void addMtxToDrawLayer(DrawLayer drawLayer, Mtx* mtx);

void drawGfx(DrawLayer layer, Gfx *toDraw);

u8* allocGfx(s32 size);

namespace gfx
{
    inline Vp viewport = {{											
        { screen_width << 1, screen_height << 1, G_MAXZ / 2, 0},
        { screen_width << 1, screen_height << 1, G_MAXZ / 2, 0},
    }};
}

constexpr uint32_t float_to_fixed(float x)
{
    return (uint32_t)(int32_t)(x * (float)0x00010000);
}

constexpr int32_t fixed_int(float a, float b)
{
    uint32_t a_fixed = float_to_fixed(a);
    uint32_t b_fixed = float_to_fixed(b);

    uint32_t a_int = a_fixed >> 16;
    uint32_t b_int = b_fixed >> 16;

    return static_cast<int32_t>((a_int << 16) | (b_int << 0));
}

constexpr int32_t fixed_frac(float a, float b)
{
    uint32_t a_fixed = float_to_fixed(a);
    uint32_t b_fixed = float_to_fixed(b);

    uint32_t a_frac = a_fixed & 0xFFFF;
    uint32_t b_frac = b_fixed & 0xFFFF;

    return static_cast<int32_t>((a_frac << 16) | (b_frac << 0));
}

constexpr Mtx float_to_fixed(const MtxF& mat)
{
    Mtx ret = 
    {{
        // Integer portion
        { fixed_int (mat[0][0], mat[0][1]), fixed_int (mat[0][2], mat[0][3]),  
          fixed_int (mat[1][0], mat[1][1]), fixed_int (mat[1][2], mat[1][3]) },
        { fixed_int (mat[2][0], mat[2][1]), fixed_int (mat[2][2], mat[2][3]),  
          fixed_int (mat[3][0], mat[3][1]), fixed_int (mat[3][2], mat[3][3]) },
        // Fractional portion
        { fixed_frac(mat[0][0], mat[0][1]), fixed_frac(mat[0][2], mat[0][3]),  
          fixed_frac(mat[1][0], mat[1][1]), fixed_frac(mat[1][2], mat[1][3]) },
        { fixed_frac(mat[2][0], mat[2][1]), fixed_frac(mat[2][2], mat[2][3]),  
          fixed_frac(mat[3][0], mat[3][1]), fixed_frac(mat[3][2], mat[3][3]) },
    }};
    return ret;
}

#endif
