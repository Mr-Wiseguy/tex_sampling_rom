#ifndef __AIFC_H__
#define __AIFC_H__

#include <cstring>
#include <array>
#include <algorithm>
#include <string_view>
#include <span>
#include <bit>

#include "bswap.h"

// http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/AIFF/Docs/AIFF-C.9.26.91.pdf

using ID = std::array<char, 4>;
using OSType = std::array<char, 4>;
using MarkerId = int16_t;

namespace std
{
    template <> struct hash<std::array<char, 4>>
    {
        constexpr size_t operator()(const ID& x) const
        {
            return std::bit_cast<uint32_t>(x);
        }
    };
}

constexpr size_t hash_ID(const ID& id)
{
    return std::hash<ID>{}(id);
}

constexpr size_t hash_ID(const std::string_view& id)
{
    char buf[4];
    std::copy_n(id.begin(), std::min(id.size(), size_t(4)), &buf[0]);
    return std::bit_cast<uint32_t>(buf);
}

constexpr size_t hash_OSType(const OSType& id)
{
    return hash_ID(id);
}

constexpr size_t hash_OSType(const std::string_view& id)
{
    return hash_ID(id);
}

struct AifcHeader {
    ID ckID;        // The text "FORM"
    uint32_t ckDataSize; // Should be the size of the form chunk but is incorrect on SDK-produced AIFC files
    ID formType;    // The text "AIFC"
    uint8_t ckData[];    // The data for the AIFF-C file
    void swap_endianness() noexcept
    {
        ckDataSize = ::swap_endianness(ckDataSize);
    }
};

struct AifcChunkHeader {
    ID ckID;        // e.g. COMM, INST, APPL
    uint32_t ckDataSize; // The size of the data array
    void swap_endianness() noexcept
    {
        ckDataSize = ::swap_endianness(ckDataSize);
    }
};

static_assert(sizeof(AifcChunkHeader) == 8, "Aifc chunk header must be 8 bytes");

// 80-bit float representation
struct float80_t {
    std::array<uint8_t, 10> bytes;
    void swap_endianness() noexcept
    {
        std::reverse(bytes.begin(), bytes.end());
    }
    long double value() noexcept
    {
        uint8_t buffer[sizeof(long double)];
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, bytes.data(), sizeof(bytes));
        return std::bit_cast<long double>(buffer);
    }
};

static_assert(sizeof(float80_t) == 10, "float80_t must be 10 bytes (80 bits)");

consteval size_t float80_test()
{
    // Bytes of an 80-bit float (padded to 16 bytes) representing the value 16000.0
    constexpr uint8_t float80_test_bytes[] {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFA, 0x0C, 0x40,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    long double ret = std::bit_cast<long double>(float80_test_bytes);
    return static_cast<size_t>(ret);
}

static_assert(float80_test() == 16000, "Long double must represent an 80-bit float");

/////////////////////////
// Common chunk (COMM) //
/////////////////////////
#pragma pack(push)
#pragma pack(1)
struct CommonChunk {
    int16_t numChannels;
    uint32_t numSampleFrames;
    int16_t sampleSize;
    float80_t sampleRate;
    ID compressionType;
    uint8_t compressionNameLen;
    char compressionName[];
    void swap_endianness() noexcept
    {
        numChannels = ::swap_endianness(numChannels);
        numSampleFrames = ::swap_endianness(numSampleFrames);
        sampleSize = ::swap_endianness(sampleSize);
        sampleRate.swap_endianness();
    }
};
#pragma pack(pop)

static_assert(sizeof(CommonChunk) == 23, "COMM chunks must be 17 bytes (not counting compression name)");

struct Loop {
    int16_t playMode;
    MarkerId beginLoop;
    MarkerId endLoop;
    void swap_endianness() noexcept
    {
        playMode = ::swap_endianness(playMode);
        beginLoop = ::swap_endianness(beginLoop);
        endLoop = ::swap_endianness(endLoop);
    }
};

static_assert(sizeof(Loop) == 6, "Loop must be 6 bytes");

/////////////////////////////
// Instrument chunk (INST) //
/////////////////////////////
struct InstrumentChunk {
    int8_t baseNote;
    int8_t detune;
    int8_t lowNote;
    int8_t highNote;
    int8_t lowVelocity;
    int8_t highVelocity;
    int16_t gain;
    Loop sustainLoop;
    Loop releaseLoop;
    void swap_endianness() noexcept
    {
        gain = ::swap_endianness(gain);
        sustainLoop.swap_endianness();
        releaseLoop.swap_endianness();
    }
};

static_assert(sizeof(InstrumentChunk) == 20, "INST chunks must be 20 bytes");

/////////////////////////////
// Sound Data Chunk (SSND) //
/////////////////////////////
struct SoundDataChunk {
    uint32_t offset; // Seemingly always zero for SDK AIFC files
    uint32_t blockSize; // Seemingly always zero for SDK AIFC files
    uint8_t soundData[];
    void swap_endianness() noexcept
    {
        offset = ::swap_endianness(offset);
        blockSize = ::swap_endianness(blockSize);
    }
};

static_assert(sizeof(SoundDataChunk) == 8, "SSND chunks must be 8 bytes");

////////////////////////////////
// Application Specific Chunk //
////////////////////////////////
struct StocApplicationSpecificChunk {
    OSType applicationSignature; // stoc
    uint8_t applicationNameLength;
    char applicationName[];
    void swap_endianness() noexcept
    {
    }
};

#endif
