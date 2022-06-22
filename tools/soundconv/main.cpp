#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <vector>
#include <unordered_set>

#include <fmt/core.h>

#include "wave.h"
#include "fx.h"
#include "aifc.h"

namespace fs = std::filesystem;
using namespace std::string_literals;

std::unordered_set sample_extensions {
    ".wav"s,
    ".ogg"s,
    ".aiff"s,
    ".aifc"s
};


std::size_t number_of_files_in_directory(const fs::path& path)
{
    using fs::directory_iterator;
    using fp = bool (*)( const fs::path&);
    return std::count_if(directory_iterator(path), directory_iterator{}, (fp)fs::is_regular_file);
}

std::vector<fs::path> get_sample_files(const fs::path& sample_folder)
{
    std::vector<fs::path> ret{};
    ret.reserve(number_of_files_in_directory(sample_folder));

    for (const auto& cur_path : fs::recursive_directory_iterator(sample_folder))
    {
        if (fs::is_regular_file(cur_path))
        {
            fs::path extension = cur_path.path().extension();
            // fmt::print("{}\n", extension.string());

            if (sample_extensions.contains(extension.string()))
            {
                ret.push_back(cur_path.path());
            }
        }
    }

    // fmt::print("Before sorting\n");
    // for (auto& val : ret)
    // {
    //     fmt::print("  {}\n", val.string());
    // }

    std::sort(ret.begin(), ret.end());
    
    // fmt::print("After sorting\n");
    // for (auto& val : ret)
    // {
    //     fmt::print("  {}\n", val.string());
    // }

    return ret;
}

int main(int argc, char * argv[])
{
    if (argc != 4)
    {
        fmt::print("Usage: {} [Sample Folder] [Effects Json] [Output Folder]\n", argv[0]);
        fmt::print("  Writes sounds.ptr, sounds.wbk, and fxbank.bin to the output folder\n");
        return EXIT_SUCCESS;
    }

    fs::path input_dir = argv[1];
    fs::path effects_json_path = argv[2];
    fs::path output_dir = argv[3];
    fs::path ptrbank_file = output_dir / "sounds.ptr";
    fs::path wavetable_file = output_dir / "sounds.wbk";
    fs::path fxbank_file = output_dir / "fxbank.bin";

    fs::path intermediate_dir = output_dir / "samples";
    fs::create_directory(intermediate_dir);
    
    std::vector<fs::path> sample_files = get_sample_files(input_dir);
    std::vector<fs::path> sample_files_relative{};
    sample_files_relative.reserve(sample_files.size());

    size_t num_input_files = sample_files.size();
    
    // Allocate and initialize one Wave for each input file
    WaveBank output(num_input_files);

    for (size_t i = 0; i < num_input_files; i++)
    {
        const fs::path& cur_path = sample_files[i];
        fs::path aifc_path;
        fs::path relative_path = fs::relative(cur_path, input_dir);
        if (cur_path.extension() == ".aifc")
        {
            aifc_path = cur_path;
        }
        else
        {
            fs::path cur_filename = cur_path.filename();
            fs::path aiff_path;
            if (cur_path.extension() == ".aiff")
            {
                aiff_path = cur_path;
            }
            else
            {
                aiff_path = intermediate_dir / cur_filename;
                aiff_path.replace_extension(".aiff");
                // fmt::print("sox -R {} -r 22500 {}", cur_path.string(), aiff_path.string());
                system(fmt::format("sox -R {} -r 22500 {}", cur_path.string(), aiff_path.string()).c_str());
            }
            fs::path table_path = intermediate_dir / cur_filename;
            table_path.replace_extension(".table");
            aifc_path = table_path;
            aifc_path.replace_extension(".aifc");
            // fmt::print("tabledesign {} > {}", aiff_path.string(), table_path.string());
            system(fmt::format("tabledesign {} > {}", aiff_path.string(), table_path.string()).c_str());
            // fmt::print("vadpcm_enc -c {} {} {}", table_path.string(), aiff_path.string(), aifc_path.string());
            system(fmt::format("vadpcm_enc -c {} {} {}", table_path.string(), aiff_path.string(), aifc_path.string()).c_str());
        }
        try
        {
            output.wave(i).read_aifc(aifc_path.c_str());
        }
        catch(const std::exception& e)
        {
            fmt::print(stderr, "Failed to read {}: {}\n", aifc_path.string(), e.what());
            return EXIT_FAILURE;
        }
        sample_files_relative.emplace_back(std::move(relative_path));
    }
    
    output.write(ptrbank_file, wavetable_file);

    FxBank fx_bank(effects_json_path);
    fx_bank.write(fxbank_file, sample_files_relative);
    
    return EXIT_SUCCESS;
}
