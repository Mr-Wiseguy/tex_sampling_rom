#ifndef __FX_H__
#define __FX_H__

#include <cstdint>
#include <vector>
#include <filesystem>
#include "bswap.h"
#include "dynamic_array.h"

struct EffectEntry {
    // Offset where the effect's track data is located
    uint32_t data_offset;
    // Sound effect's priority
    uint32_t priority;
    void swap_endianness() noexcept
    {
        ::swap_endianness_inplace(data_offset);
        ::swap_endianness_inplace(priority);
    }
};

struct FxBankHeader {
    // Length of the effects array (TODO why is this different than num_effects)
    uint32_t num_components;
    // Number of effects in this fx bank
    uint32_t num_effects;
    // Number of waves in this fx bank's wavetable
    uint32_t num_waves;
    // Exported as zero, used at runtime
    uint32_t flags;
    // Exported as zero, used at runtime
    uint32_t ptr_addr;
    // Offset where the wave table is located
    uint32_t wave_table_offset;
    void swap_endianness() noexcept
    {
        ::swap_endianness_inplace(num_components);
        ::swap_endianness_inplace(num_effects);
        ::swap_endianness_inplace(num_waves);
        ::swap_endianness_inplace(wave_table_offset);
    }
};

struct Envelope {
    uint8_t envelope_speed;
    uint8_t initial_volume;
    uint8_t attack_speed;
    uint8_t peak_volume;
    uint8_t decay_speed;
    uint8_t sustain_volume;
    uint8_t release_speed;
};

class Effect {
public:
    Effect() = default;
    Effect(const std::filesystem::path& sample_path) :
        sample_path_(sample_path) {}
    const std::filesystem::path& sample_path() const { return sample_path_; }
private:
    std::filesystem::path sample_path_;
    Envelope envelope_;
    int pan_;
    int volume_;
    bool set_envelope_;
    bool set_pan_;
    bool set_volume_;
};

class FxBank {
public:
    FxBank(const std::filesystem::path& fx_json_path);
    void write(const std::filesystem::path& output_path, const std::vector<std::filesystem::path>& sample_paths);
private:
    dynamic_array<Effect> effects_;
};

#endif
