#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstddef>

#include <fmt/core.h>
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>

#include "wave.h"
#include "fx.h"
#include "bswap.h"
#include "dynamic_array.h"

namespace fs = std::filesystem;
namespace js = nlohmann;

// Rounds the input up to the nearest N, where N is a power of 2
template <size_t N, typename T>
constexpr T round_up(T in)
{
    static_assert(N && !(N & (N - 1)), "Can only round up to the nearest multiple of a power of 2!");
    return (in + N - 1) & -N;
}

template <size_t N>
constexpr std::streampos round_up(std::streampos in)
{
    static_assert(N && !(N & (N - 1)), "Can only round up to the nearest multiple of a power of 2!");
    return (in + std::streamoff{N - 1}) & -N;
}

void FxBank::write(const fs::path& output_path, const std::vector<fs::path>& sample_paths)
{
    std::ofstream output_file(output_path, std::ios::binary);

    FxBankHeader header;
    header.num_components = effects_.size();
    header.num_effects = effects_.size();

    header.flags = 0;
    header.ptr_addr = 0;

    // Allocate a vector to hold the current effect's sequence data
    std::vector<uint8_t> seq_data{};
    seq_data.reserve(32);
    // Allocate a vector to hold the fx bank's wave table
    // This is a list of the wave indices from the pointer bank that are used in the fx bank
    std::vector<fs::path> wave_paths{};
    std::vector<int16_t> wave_table{};
    wave_paths.reserve(sample_paths.size());
    wave_table.reserve(sample_paths.size());
    // Create an array to hold the fx array that will be written to the fx bank file
    dynamic_array<EffectEntry> fx_array(effects_.size());
    // Seek past the header and fx array in the file to prepare for writing effect sequence data
    output_file.seekp(sizeof(FxBankHeader) + sizeof(EffectEntry) * fx_array.size());
    // Write each effect's sequence data
    for (size_t effect_idx = 0; effect_idx < effects_.size(); effect_idx++)
    {
        // Get the effect being written to the file
        const Effect& cur_effect = effects_[effect_idx];

        // Record the position in the file as the effect's sequence data offset
        fx_array[effect_idx].priority = 0x64;
        fx_array[effect_idx].data_offset = output_file.tellp();
        fx_array[effect_idx].swap_endianness();

        // Get the sample used by this effect and calculate the corresponding wave index
        const fs::path& effect_sample = cur_effect.sample_path();
        int wave_idx;
        // Check if the sample is already in the wave table
        // If it isn't, add it to the wave table
        auto find_result = std::find(wave_paths.begin(), wave_paths.end(), effect_sample);
        if (find_result == wave_paths.end())
        {
            auto ptrbank_find_result = std::find(sample_paths.begin(), sample_paths.end(), effect_sample);
            if (ptrbank_find_result == sample_paths.end())
            {
                fmt::print(stderr, "Sample {} used in sound effect does not exist!\n", effect_sample.string());
                output_file.close();
                fs::remove(output_path);
                std::exit(EXIT_FAILURE);
            }
            int ptrbank_wave_idx = std::distance(sample_paths.begin(), ptrbank_find_result);
            wave_idx = wave_paths.size();
            wave_paths.push_back(effect_sample);
            wave_table.push_back(::swap_endianness(static_cast<int16_t>(ptrbank_wave_idx)));
        }
        // If it is, get the corresponding wave index
        else
        {
            wave_idx = std::distance(wave_paths.begin(), find_result);
        }

        // Create the sequence data
        seq_data.clear();

        // Fwave(wave_idx)
        seq_data.push_back(0x81); seq_data.push_back(wave_idx);
        // Fdefa(0x01, 0x00, 0x01, 0x72, 0x46, 0x6F, 0x7C)
        seq_data.push_back(0x84); seq_data.push_back(0x01);
        seq_data.push_back(0x00); seq_data.push_back(0x01);
        seq_data.push_back(0x72); seq_data.push_back(0x46);
        seq_data.push_back(0x6F); seq_data.push_back(0x7C);
        // Fpan(0x7F)
        seq_data.push_back(0x9C); seq_data.push_back(0x7F);
        // Fvolume(180)
        seq_data.push_back(0xA6); seq_data.push_back(180);
        // Note(0x30, 0x7FFF)
        seq_data.push_back(0x30); seq_data.push_back(0xFF);
        seq_data.push_back(0xFF);
        // Fstop()
        seq_data.push_back(0x80);

        // Write the sequence data
        output_file.write(reinterpret_cast<const char*>(seq_data.data()), seq_data.size());
    }

    // Write the wave table
    header.num_waves = wave_table.size();
    header.wave_table_offset = round_up<2>(output_file.tellp());
    output_file.seekp(header.wave_table_offset);
    output_file.write(reinterpret_cast<const char*>(wave_table.data()), wave_table.size() * sizeof(wave_table[0]));

    // Write the fx bank header
    header.swap_endianness();
    output_file.seekp(0);
    output_file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Write the fx array
    output_file.write(reinterpret_cast<const char*>(fx_array.data()), fx_array.size() * sizeof(fx_array[0]));
}

FxBank::FxBank(const fs::path& fx_json_path)
{
    js::json input_json;
    try
    {
        std::ifstream input_file(fx_json_path);
        input_file >> input_json;
    }
    catch (js::json::exception& err)
    {
        fmt::print("Error parsing json: {}\n", err.what());
        std::exit(EXIT_FAILURE);
    }

    const auto& sfx_array = input_json["sfx"];
    size_t num_sfx = sfx_array.size();

    dynamic_array<Effect> effects(num_sfx);

    std::transform(sfx_array.begin(), sfx_array.end(), effects.begin(),
        [&](const auto& cur_effect)
        {
            const std::string& cur_effect_path = cur_effect["sample"];
            const std::string& cur_effect_name = cur_effect["name"];
            return Effect{fs::path(cur_effect_path)};
        }
    );

    effects_ = std::move(effects);
}