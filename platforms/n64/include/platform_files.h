#ifndef __PLATFORM_FILES_H__
#define __PLATFORM_FILES_H__

#include <cstdint>
#include <memory>
#include <utility>

struct LoadRxSlot;

// Deleter class for use with load receive slots
class LoadRxSlotDeleter
{
public:
    void operator()(void* ptr);
};

class LoadHandle
{
public:
    // Default constructor
    LoadHandle() = default;
    // Default destructor thanks to unique_ptr
    ~LoadHandle() = default;
    // Not copyable or copy assignable
    LoadHandle(const LoadHandle&) = delete;
    LoadHandle& operator=(const LoadHandle&) = delete;
    // Move constructor
    LoadHandle(LoadHandle&& rhs) :
        handle_slot_(std::exchange(rhs.handle_slot_, nullptr))
    {}
    // Move assignment operator
    LoadHandle& operator=(LoadHandle&& rhs)
    {
        handle_slot_ = std::exchange(rhs.handle_slot_, nullptr);
        return *this;
    }

    // Checks if the corresponding load is complete
    bool is_finished();
    // Waits for the corresponding load to complete
    void* join();
private:
    std::unique_ptr<LoadRxSlot, LoadRxSlotDeleter> handle_slot_;
    LoadHandle(LoadRxSlot *handle_slot) :
        handle_slot_(handle_slot)
    {}

    friend LoadHandle start_data_load(void*, uint32_t, uint32_t);
};

#endif
