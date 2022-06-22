#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>

#include <fmt/core.h>

#include "wave.h"
#include "bswap.h"
#include "dynamic_array.h"

namespace fs = std::filesystem;

struct PointerTableHeader {
    // Title string ("N64 PtrTablesV2")
    char title[16];
    // Unused flags field
    uint32_t unused;
    // Unused string
    char unused2[12];
    // Number of waves in this wave table
    uint32_t waveCount;
    // Offset where the basenote table is located
    uint32_t basenoteOffset;
    // Offset where the detune table is located
    uint32_t detuneOffset;
    // Offset where the wave list is located
    uint32_t waveListOffset;
    void swap_endianness() noexcept
    {
        waveCount = ::swap_endianness(waveCount);
        basenoteOffset = ::swap_endianness(basenoteOffset);
        detuneOffset = ::swap_endianness(detuneOffset);
        waveListOffset = ::swap_endianness(waveListOffset);
    }
};

struct ADPCMWaveEntry {
    // Offset in the wave table for this wave's data
    uint32_t dataOffset; 
    // Length in the wave table of this wave's data
    uint32_t dataLength;
    // Data type (0 = ADPCM, 1 = 16-bit PCM)
    uint8_t dataType;
    // Unused flags byte
    uint8_t unused;
    // Padding (not strictly needed, compiler would insert automatically)
    uint8_t padding, padding2;
    // Offset into the ptrtable for this wave's loop data
    uint32_t loopOffset;
    // Offset into the ptrtable for this wave's codebook
    uint32_t codebookOffset;
    void swap_endianness() noexcept
    {
        dataOffset = ::swap_endianness(dataOffset);
        dataLength = ::swap_endianness(dataLength);
        loopOffset = ::swap_endianness(loopOffset);
        codebookOffset = ::swap_endianness(codebookOffset);
    }
};

struct CodeBook {
    uint32_t order;
    uint32_t npredictors;
    int16_t data[];
    void swap_endianness() noexcept
    {
        order = ::swap_endianness(order);
        npredictors = ::swap_endianness(npredictors);
    }
};

struct PCMWaveEntry {
    // Offset in the wave table for this wave's data
    uint32_t dataOffset; 
    // Length in the wave table of this wave's data
    uint32_t dataLength;
    // Data type (0 = ADPCM, 1 = 16-bit PCM)
    uint8_t dataType;
    // Unused flags byte
    uint8_t unused;
    // Padding (not strictly needed, compiler would insert automatically)
    uint8_t padding, padding2;
    // Offset into the ptrtable for this wave's loop data
    uint32_t loopOffset;
    void swap_endianness() noexcept
    {
        dataOffset = ::swap_endianness(dataOffset);
        dataLength = ::swap_endianness(dataLength);
        loopOffset = ::swap_endianness(loopOffset);
    }
};

using basenote_t = uint8_t;
using detune_t = float;

size_t wave_size(const Wave& wave)
{
    // TODO support raw PCM waves
    return
        // Wave entry in file
        sizeof(ADPCMWaveEntry) +
        // Padding
        0x04 + 
        // Codebook Header
        sizeof(CodeBook) + 
        // Codebook data
        sizeof(int16_t) * wave.codebook_length();
}

constexpr size_t align_pow2(size_t input, size_t alignment)
{
    return (input + (alignment -  1)) & ~(alignment - 1);
}

constexpr size_t write_buf_size = 4096;

void WaveBank::write(const fs::path& ptrtable, const fs::path& wavetable)
{
    uint32_t numWaves = waves_.size();
    // Offset of every wave's data in the wavetable
    dynamic_array<uint32_t> waveOffsets(numWaves);
    // Length of every wave's data in the wavetable
    dynamic_array<uint32_t> waveLengths(numWaves);
    // Allocate a buffer for file writing to increase performance
    std::unique_ptr<char[]> writeBuf = std::make_unique<char[]>(write_buf_size);

    // Write wavetable
    {
        std::ofstream wavetableFile(wavetable, std::ios_base::binary);
        wavetableFile.rdbuf()->pubsetbuf(writeBuf.get(), write_buf_size);

        // Write the header (and seek to after it just in case)
        wavetableFile.write("N64 WaveTables ", 16);
        wavetableFile.seekp(16);

        // Write wave data to wavetable
        for (size_t i = 0; i < numWaves; i++)
        {
            const Wave& wave = waves_[i];

            waveOffsets[i] = wavetableFile.tellp();

            wavetableFile.write(reinterpret_cast<char*>(wave.sample_data().data()), wave.sample_data().size_bytes());

            waveLengths[i] = static_cast<uint32_t>(wavetableFile.tellp()) - waveOffsets[i];
        }
    }
    // Write ptrtable
    {
        std::ofstream ptrtableFile(ptrtable, std::ios_base::binary);
        ptrtableFile.rdbuf()->pubsetbuf(writeBuf.get(), write_buf_size);

        // Skip the header, will get written after the rest of the data is written
        ptrtableFile.seekp(sizeof(PointerTableHeader), std::ios_base::beg);

        dynamic_array<uint32_t> waveEntryOffsets(numWaves);

        // Write wave entries
        for (size_t i = 0; i < numWaves; i++)
        {
            const Wave& wave = waves_[i];

            // Record the position for writing the wave list later
            uint32_t waveEntryPos = ptrtableFile.tellp();
            waveEntryOffsets[i] = swap_endianness(waveEntryPos);

            uint32_t codePos = waveEntryPos + sizeof(ADPCMWaveEntry);
            codePos = align_pow2(codePos, 8); // Always seemingly aligned to 8

            uint32_t loopPos = codePos + sizeof(CodeBook) + sizeof(int16_t) * wave.codebook_length();
            loopPos = align_pow2(loopPos, 8); // Not strictly needed

            uint32_t nextPos = wave.has_loop_data()
                ? align_pow2(loopPos + sizeof(LoopData), 8)
                : loopPos;
            
            ADPCMWaveEntry curEntry {
                waveOffsets[i],
                waveLengths[i],
                0, // ADPCM
                0, // unused
                0, 0, // padding
                wave.has_loop_data() ? loopPos : 0,
                codePos
            };
            curEntry.swap_endianness();
            ptrtableFile.write(reinterpret_cast<char*>(&curEntry), sizeof(curEntry));

            CodeBook curBook {
                wave.codebook_order(),
                wave.codebook_npredictors()
            };
            
            curBook.swap_endianness();
            ptrtableFile.seekp(codePos, std::ios::beg);
            ptrtableFile.write(reinterpret_cast<char*>(&curBook), sizeof(curBook));
            ptrtableFile.write(reinterpret_cast<char*>(wave.codebook_data().data()), sizeof(int16_t) * wave.codebook_length());

            if (wave.has_loop_data())
            {
                ptrtableFile.seekp(loopPos, std::ios::beg);
                ptrtableFile.write(reinterpret_cast<const char*>(&wave.loop_data()), sizeof(LoopData));
            }
            
            ptrtableFile.seekp(nextPos, std::ios::beg);

            // fmt::print("Wrote wave entry at 0x{:04x} of length 0x{:04x} ending at 0x{:04x}\n", waveEntryPos, nextPos - waveEntryPos, nextPos);
        }

        // Write the wave list
        uint32_t wavelistPos = ptrtableFile.tellp();
        ptrtableFile.write(reinterpret_cast<char*>(waveEntryOffsets.data()), numWaves * sizeof(uint32_t));
        
        // Allocate array to hold a bunch of zeroes so each array can be written in a single operation
        std::unique_ptr<char[]> zeroes = std::make_unique<char[]>(numWaves * sizeof(detune_t));

        // Write basenote array
        uint32_t basenotePos = ptrtableFile.tellp();
        // Write the basenotes, plus extra zeroes to keep the detune array aligned
        ptrtableFile.write(zeroes.get(), align_pow2(numWaves * sizeof(basenote_t), sizeof(detune_t)));

        // Write detune array
        uint32_t detunePos = ptrtableFile.tellp();
        ptrtableFile.write(zeroes.get(), numWaves * sizeof(detune_t));

        // Rewind and write the file header
        PointerTableHeader header {
            "N64 PtrTablesV2",
            0, // unused
            "", // unused
            numWaves,
            basenotePos,
            detunePos,
            wavelistPos
        };
        header.swap_endianness();

        ptrtableFile.seekp(0, std::ios_base::beg);
        ptrtableFile.write(reinterpret_cast<char*>(&header), sizeof(header));
    }
    // fmt::print("Wrote files: {} {}\n", ptrtable.c_str(), wavetable.c_str());
}
