#include <fstream>
#include <algorithm>

#include <fmt/core.h>

#include "wave.h"
#include "aifc.h"

constexpr size_t file_buffer_size = 1024 * 1024;

const std::string exc_file_does_not_exist = "File does not exist!";
const std::string exc_invalid_file = "Not a valid aifc file!";
const std::string exc_malformed_file = "File is corrupt!";
const std::string exc_incorrect_compression = "Incorrect compression type!";
const std::string exc_incorrect_sample_size = "Sample depth must be 16-bit!";
const std::string exc_invalid_data = "Invalid data found in aifc file!";
const std::string exc_invalid_code_data = "Invalid codebook data found in aifc file!";
const std::string exc_invalid_loop_data = "Invalid loop data found in aifc file!";
const std::string exc_incomplete_file = "Incomplete aifc file!";
const std::string exc_multiple_loops = "Aifc has multiple loops; only one is supported!";

void Wave::process_common_chunk(std::span<char> data)
{
    // Read the chunk header and correct endianness
    CommonChunk *chunk = reinterpret_cast<CommonChunk*>(data.data());
    chunk->swap_endianness();

    // Ensure that this is a VADPCM aifc file
    if (hash_ID(chunk->compressionType) != hash_ID("VAPC"))
    {
        throw std::invalid_argument(exc_incorrect_compression);
    }

    // Ensure that the original samples were 16 bit PCM
    if (chunk->sampleSize != 16)
    {
        throw std::invalid_argument(exc_incorrect_sample_size);
    }

    sampleRate_ = static_cast<uint32_t>(chunk->sampleRate.value());
    sampleCount_ = chunk->numSampleFrames;
    foundCommon_ = true;
}

void Wave::process_instrument_chunk(std::span<char> data)
{
    // Read the chunk header and correct endianness
    InstrumentChunk *chunk = reinterpret_cast<InstrumentChunk*>(data.data());
    chunk->swap_endianness();

    // Nothing needed here, instrument parameters pulled from instrument data (e.g. sfz)

    foundInstrument_ = true;
}

void Wave::process_application_chunk(std::span<char> data)
{
    // Read the chunk header and correct endianness
    StocApplicationSpecificChunk *chunk = reinterpret_cast<StocApplicationSpecificChunk*>(data.data());
    chunk->swap_endianness();

    // Check that the application signature is "stoc", which it will always be for aifc files from the SDK
    if (hash_OSType(chunk->applicationSignature) != hash_OSType("stoc"))
    {
        throw std::invalid_argument(exc_invalid_data);
    }

    char *applicationData = data.data() + sizeof(StocApplicationSpecificChunk) + chunk->applicationNameLength;
    size_t applicationDataLength = data.data() + data.size() - applicationData;

    // Codebook Data
    if (strncmp(chunk->applicationName, "VADPCMCODES", chunk->applicationNameLength) == 0)
    {
        VadpcmCodes* codes = reinterpret_cast<VadpcmCodes*>(applicationData);
        codes->swap_endianness();

        size_t bookLength = 8 * codes->npredictors * codes->order;

        // Verify data length is correct
        if (applicationDataLength != sizeof(VadpcmCodes) + bookLength * sizeof(int16_t))
        {
            throw std::invalid_argument(exc_invalid_code_data);
        }

        // Copy codebook into wave
        order_ = codes->order;
        npredictors_ = codes->npredictors;
        book_ = std::make_unique<int16_t[]>(bookLength);
        std::copy(&codes->data[0], &codes->data[bookLength], book_.get());
        foundBookData_ = true;
    }
    // Loop Data
    else if (strncmp(chunk->applicationName, "VADPCMLOOPS", chunk->applicationNameLength) == 0)
    {
        // Read the loops data and correct endianness
        VadpcmLoops* loops = reinterpret_cast<VadpcmLoops*>(applicationData);
        loops->swap_endianness();

        // Only one loop is supported
        if (loops->nloops != 1)
        {
            throw std::invalid_argument(exc_multiple_loops);
        }

        // Verify data length is correct
        if (applicationDataLength != sizeof(VadpcmLoops))
        {
            throw std::invalid_argument(exc_invalid_loop_data);
        }
        
        // Copy loop data into the wave
        loopData_ = loops->loopData;
        foundLoopData_ = true;
    }
    else
    {
        // Invalid file due to unknown application name
        throw std::invalid_argument(exc_invalid_data);
    }

    foundInstrument_ = true;
}

void Wave::process_sounddata_chunk(std::span<char> data)
{
    // Read the chunk header and correct endianness
    SoundDataChunk *chunk = reinterpret_cast<SoundDataChunk*>(data.data());
    chunk->swap_endianness();

    // Calculate the number of bytes of sample data from the chunk length
    size_t sampleBytes = data.size_bytes() - sizeof(SoundDataChunk);
    sampleDataLen_ = sampleBytes;
    // Allocate memory to hold the sample data and copy it from the chunk
    sampleData_ = std::make_unique<std::byte[]>(sampleBytes);
    memcpy(sampleData_.get(), chunk->soundData, sampleBytes);

    foundSoundData_ = true;
}

void Wave::read_aifc(const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath))
    {
        throw std::invalid_argument(exc_file_does_not_exist);
    }
    // Allocate a buffer to hold the file's contents
    uintmax_t filesize = std::filesystem::file_size(filepath);
    std::unique_ptr<char[]> filebuf = std::make_unique<char[]>(filesize);

    // Open the file and read it into the buffer
    {
        std::ifstream aifcFile(filepath, std::ios::binary);
        aifcFile.read(filebuf.get(), filesize);
    }

    char *curPos = filebuf.get();
    char *endPos = curPos + filesize;
    AifcHeader *header = reinterpret_cast<AifcHeader*>(curPos);
    curPos += sizeof(AifcHeader);
    header->swap_endianness();

    // Check that the file header is correct
    if (
        !std::equal(header->ckID.begin(), header->ckID.end(), "FORM") ||
        !std::equal(header->formType.begin(), header->formType.end(), "AIFC"))
    {
        throw std::invalid_argument(exc_invalid_file);
    }

    // Read chunks until we reach the end of the file
    while (curPos < endPos)
    {
        // Get the current chunk's header and advance the file position
        AifcChunkHeader *chunkHeader = reinterpret_cast<AifcChunkHeader*>(curPos);
        curPos += sizeof(AifcChunkHeader);
        chunkHeader->swap_endianness();
        
        // Check that the remaining data in the file is at least as long as the chunk length
        // If it isn't, then the file is malformed
        ptrdiff_t remainingSize = endPos - curPos;
        if (remainingSize < chunkHeader->ckDataSize)
        {
            throw std::invalid_argument(exc_malformed_file);
        }

        std::span<char> chunkDataSpan(curPos, chunkHeader->ckDataSize);

        // Process the chunk
        switch (hash_ID(chunkHeader->ckID))
        {
            case hash_ID("COMM"):
                process_common_chunk(chunkDataSpan);
                break;
            case hash_ID("INST"):
                process_instrument_chunk(chunkDataSpan);
                break;
            case hash_ID("APPL"):
                process_application_chunk(chunkDataSpan);
                break;
            case hash_ID("SSND"):
                process_sounddata_chunk(chunkDataSpan);
                break;
            default:
                fmt::print(stderr, "Unknown chunk: {:<.4}\n", chunkHeader->ckID.begin());
                std::exit(EXIT_FAILURE);
        }

        // Advance to the next chunk
        curPos += chunkHeader->ckDataSize;
    }

    // Check if all of the required data was in the file
    // Instrument data isn't used and loop data is optional, so only these are needed
    if (!foundCommon_ || !foundSoundData_ || !foundBookData_)
    {
        throw std::invalid_argument(exc_incomplete_file);
    }

    // fmt::print("Parsed file {}\n", filepath.c_str());
    // fmt::print("  Sample Rate:  {}\n", sampleRate_);
    // fmt::print("  Sample Count: {}\n", sampleCount_);
    // fmt::print("  Sample Bytes: {}\n", sampleDataLen_);
    // fmt::print("  Sample Data:  0x{:02x} 0x{:02x} ...\n", sampleData_[0], sampleData_[1]);
    // fmt::print("  Codebook Length: {}\n", codebook_length());
    // fmt::print("  Codebook Data:   0x{:04x} 0x{:04x} ...\n", (uint16_t)book_[0], (uint16_t)book_[1]);
}
