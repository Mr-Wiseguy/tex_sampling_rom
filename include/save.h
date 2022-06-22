#ifndef __SAVE_H__
#define __SAVE_H__

#include <cstdint>

struct SaveData {
    uint32_t level_times[3];
    uint8_t level;
    uint32_t padding;
};

struct SaveFile {
    SaveData data;
    uint32_t magic;
};

constexpr uint32_t save_magic = 0x12345678;

extern SaveFile g_SaveFile;

void do_save();
void load_save();

#endif
