#ifndef __FILES_H__
#define __FILES_H__

#include <cstdint>
#include <cstddef>

#include <types.h>
#include <platform_files.h>

// Implementation-defined in platform_files.h
class LoadHandle;


[[nodiscard]] LoadHandle start_file_load(const char *path);
LoadHandle start_data_load(void* ret, uint32_t rom_pos, uint32_t size); // TODO refactor for PC support
[[nodiscard]] void *load_file(const char *path);
[[nodiscard]] void *load_data(uint32_t rom_pos, uint32_t size); // Same as above
void *load_data(void* ret, uint32_t rom_pos, uint32_t size); // Same as above
[[nodiscard]] Model *load_model(const char *path);
[[nodiscard]] void* get_or_load_image(const char* path);

template <typename T>
[[nodiscard]] T* load_file(const char *path)
{
    return static_cast<T*>(load_file(path));
}

#endif
