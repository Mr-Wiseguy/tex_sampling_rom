#include <cstdlib>
#include <filesystem>
#include <vector>
#include <fstream>
#include <tuple>

#include <fmt/core.h>
#include <fmt/os.h>

#include "dynamic_array.h"

namespace fs = std::filesystem;

struct size_offset_t
{
    size_t size;
    size_t offset;
};

// Alignment in bytes of file offsets
constexpr size_t file_alignment_bytes = 4;

// Rounds the input up to the nearest N, where N is a power of 2
template <size_t N, typename T>
constexpr T round_up(T in)
{
    static_assert(N && !(N & (N - 1)), "Can only round up to the nearest multiple of a power of 2!");
    return (in + N - 1) & -N;
}

size_t gather_files(const fs::path& dir, std::vector<fs::path>& files)
{
    size_t ret = 0;
    for (const auto& cur_entry : fs::directory_iterator(dir))
    {
        if (fs::is_directory(cur_entry))
        {
            ret += gather_files(cur_entry, files);
        }
        else if (fs::is_regular_file(cur_entry))
        {
            if (cur_entry.path().filename().c_str()[0] != '.')
            {
                files.emplace_back(std::move(cur_entry.path()));
                ret += round_up<file_alignment_bytes>(fs::file_size(cur_entry));
            }
        }
    }
    return ret;
}

dynamic_array<size_offset_t> write_files(std::ofstream& output_file, std::vector<fs::path>& all_files, size_t total_size)
{
    dynamic_array<size_offset_t> file_offsets(all_files.size());

    dynamic_array<char> output_buf(total_size);
    size_t cur_offset = 0;

    for (size_t file_idx = 0; file_idx < all_files.size(); file_idx++)
    {
        // Get the current file's path and size
        const std::string& cur_file_path = all_files[file_idx];
        size_t cur_file_size = fs::file_size(cur_file_path);
        // Record the file's size, rounded up to alignment
        file_offsets[file_idx].size = round_up<file_alignment_bytes>(cur_file_size);
        // Record the current output buffer position as the offset for the current file
        file_offsets[file_idx].offset = cur_offset;
        // Open the current file and read its contents into the buffer
        std::ifstream cur_file(all_files[file_idx], std::ios_base::binary);
        cur_file.read(output_buf.data() + cur_offset, cur_file_size);
        // Advance the output buffer position by the file's size, rounded up to the file alignment
        cur_offset = round_up<file_alignment_bytes>(cur_offset + cur_file_size);
    }

    output_file.write(output_buf.data(), output_buf.size());
    return file_offsets;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fmt::print("Usage: {} [asset folder] [packed asset file] [asset table]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    fs::path asset_folder(argv[1]);
    char *packed_file_path = argv[2];
    char *asset_table_path = argv[3];

    if (!fs::is_directory(asset_folder))
    {
        fmt::print(stderr, "Provided asset folder ({}) is not a valid directory\n", asset_folder.c_str());
        return EXIT_FAILURE;
    }

    std::vector<fs::path> all_files;
    size_t total_size = gather_files(asset_folder, all_files);

    std::ofstream packed_file(packed_file_path, std::ios_base::binary);
    dynamic_array<size_offset_t> file_offsets = write_files(packed_file, all_files, total_size);
    packed_file.close();

    auto asset_table = fmt::output_file(asset_table_path);
    for (size_t file_idx = 0; file_idx < file_offsets.size(); file_idx++)
    {
        asset_table.print("{}, {}, {}\n",
            fs::relative(all_files[file_idx], asset_folder).c_str(),
            file_offsets[file_idx].offset,
            file_offsets[file_idx].size);
    }
    asset_table.close();

    return EXIT_SUCCESS;
}
