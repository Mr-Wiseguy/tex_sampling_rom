#include <text.h>
#include <gfx.h>
#include <block_vector.h>
#include <main.h>

#include <ultra64.h>
#include <n64_gfx.h>

// Font modified from https://opengameart.org/content/8x8-font
__asm__(
 ".section \".rodata\", \"a\", @progbits\n"
 "font_img:\n"
 ".incbin \"./platforms/n64/src/gfx/font.bin\"\n"
 ".previous\n"
);

extern uint8_t font_img[];

constexpr int font_char_offset = ' ';
constexpr int font_img_width = 608;
constexpr int font_img_height = 8;

struct TextEntry {
    int length;
    uint16_t x, y;
    uint32_t color;
    block_vector<char>::iterator text;
};

block_vector<TextEntry> text_entries;
block_vector<char> text_storage;

uint32_t cur_color = 0x0;

void set_text_color(int r, int g, int b, int a)
{
    cur_color = (r & 0xFF) << 24 |
                (g & 0xFF) << 16 |
                (b & 0xFF) << 8 |
                (a & 0xFF) << 0;
}

void text_init()
{
    text_entries = {};
    text_storage = {};
}

void text_reset()
{
    text_entries = {};
    text_storage = {};
}

void print_text(int x, int y, char const* text, int length)
{
    text_entries.emplace_back(length, static_cast<uint16_t>(x), static_cast<uint16_t>(y), cur_color, text_storage.end());
    std::copy(text, text + length, std::back_inserter(text_storage));
}

void print_text_centered(int x, int y, char const* text, int length)
{
    int width = length * 6;
    print_text(x - width / 2, y, text, length);
}

extern Gfx* g_gui_dlist_head;

void draw_all_text()
{
    gDPPipeSync(g_gui_dlist_head++);
    gDPSetCycleType(g_gui_dlist_head++, G_CYC_1CYCLE);
    gDPSetTexturePersp(g_gui_dlist_head++, G_TP_NONE);
    gDPSetCombineLERP(g_gui_dlist_head++, ENVIRONMENT, 0, TEXEL0, 0, 0, 0, 0, TEXEL0, ENVIRONMENT, 0, TEXEL0, 0, 0, 0, 0, TEXEL0);
    gDPSetRenderMode(g_gui_dlist_head++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    gDPLoadTextureBlock_4b(g_gui_dlist_head++, font_img, G_IM_FMT_IA, font_img_width, font_img_height, 0,  0, 0, 0, 0, 0, 0);
    gDPSetTexturePersp(g_gui_dlist_head++, G_TP_NONE);
    gDPSetTextureFilter(g_gui_dlist_head++, G_TF_POINT);
    for (const auto& entry : text_entries)
    {
        gDPPipeSync(g_gui_dlist_head++);
        gDPSetColor(g_gui_dlist_head++, G_SETENVCOLOR, entry.color);
        int start_x = entry.x;
        int x = start_x;
        int y = entry.y;
        auto it = entry.text;
        for (int i = 0; i < entry.length; i++)
        {
            unsigned int character = *it;
            if (character == '\n') {
                x = start_x;
                y += 10;
            } else {
                gSPTextureRectangle(g_gui_dlist_head++, (x) << 2, (y) << 2, (x + 6) << 2, (y + 8) << 2, 0,
                    ((character - font_char_offset) * 6) << 5, 0 << 5,
                    1 << 10, 1 << 10);
                x += 6;
            }
            ++it;
        }
    }
    text_entries = {};
    text_storage = {};
}
