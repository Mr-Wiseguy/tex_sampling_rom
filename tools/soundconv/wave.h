#ifndef __WAVE_H__
#define __WAVE_H__

#include <filesystem>
#include <cstddef>
#include <memory>
#include <span>

#include "bswap.h"
#include "dynamic_array.h"

struct VadpcmCodes {
    uint16_t version;
    uint16_t order;
    uint16_t npredictors;
    int16_t data[];
    void swap_endianness()
    {
        version = bswap16(version);
        order = bswap16(order);
        npredictors = bswap16(npredictors);
    }
};

using LoopData = std::array<uint8_t, 44>;

struct VadpcmLoops {
    uint16_t version;
    uint16_t nloops;
    LoopData loopData;
    void swap_endianness()
    {
        version = bswap16(version);
        nloops = bswap16(nloops);
    }
};

class Wave {
public:
    // Reads VADPCM data from an aifc file
    void read_aifc(const std::filesystem::path& filepath);
    // Checks if this data has associated loop data
    bool has_loop_data() const noexcept { return foundLoopData_; }
    const LoopData& loop_data() const noexcept { return loopData_; }
    // Gets the length of this wave's codebook
    size_t codebook_length() const noexcept { return 8 * order_ * npredictors_; }
    uint32_t codebook_order() const noexcept { return order_; }
    uint32_t codebook_npredictors() const noexcept { return npredictors_; }
    std::span<std::int16_t> codebook_data() const noexcept { return std::span<std::int16_t>(book_.get(), codebook_length()); }
    // Gets this wave's sample count
    int32_t sample_count() const noexcept { return sampleCount_; }
    // Gets this wave's sample rate (samples per second)
    int32_t sample_rate() const noexcept { return sampleRate_; }
    // Gets the bytes of this wave's data
    std::span<std::byte> sample_data() const noexcept { return std::span<std::byte>(sampleData_.get(), sampleDataLen_); }
private:
    // Samples
    uint32_t sampleCount_;
    uint32_t sampleRate_;
    size_t sampleDataLen_;
    std::unique_ptr<std::byte[]> sampleData_;
    // Codebook
    uint32_t order_;
    uint32_t npredictors_;
    std::unique_ptr<std::int16_t[]> book_; // Always big-endian
    // Loop
    LoopData loopData_;
    // Keeping track of which data has been found in the input file
    bool foundCommon_;
    bool foundInstrument_;
    bool foundSoundData_;
    bool foundBookData_;
    bool foundLoopData_;

    void process_common_chunk(std::span<char> data);
    void process_instrument_chunk(std::span<char> data);
    void process_sounddata_chunk(std::span<char> data);
    void process_application_chunk(std::span<char> data);
};

class WaveBank {
public:
    WaveBank(size_t num_waves) :
        waves_(num_waves) {}
    Wave& wave(size_t idx) noexcept { return waves_[idx]; }
    void write(const std::filesystem::path& ptrbank, const std::filesystem::path& wavebank);
private:
    dynamic_array<Wave> waves_;
};

#endif