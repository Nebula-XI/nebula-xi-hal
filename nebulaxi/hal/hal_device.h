#pragma once

#ifdef __linux__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#else
// clang-format off
#include <windows.h>
#include <initguid.h>
#include <setupapi.h>
// clang-format on
#endif

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace nebulaxi {

struct device_info {
    uint32_t vendor;
    uint32_t device;
    uint32_t bus;
    uint32_t dev;
    uint32_t func;
};

struct device_file {
    int handle {};
    mutable std::mutex mutex;
};

using device_path = std::string;
using device_path_info_list = std::vector<std::pair<const device_path, const device_info>>;

using reg_offset = std::size_t;
using reg_value = uint32_t;

class hal_device final {

    inline static constexpr char io_name[] { "DEV" };

public:
    hal_device() = delete;

    static device_path_info_list get_path_info_list();

#ifdef __linux__
    template <const char* io_name = io_name>
    static reg_value read(const device_file& file, reg_offset offset)
    {
        using namespace std::string_literals;
        const std::lock_guard<std::mutex> lock(file.mutex);
        reg_value value {};
        auto result_read = pread(file.handle, &value, sizeof(value), offset);
        if (result_read != sizeof(value)) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Invalid read data");
        }
        return value;
    }

    template <const char* io_name = io_name>
    static void write(const device_file& file, reg_offset offset, reg_value value)
    {
        using namespace std::string_literals;
        const std::lock_guard<std::mutex> lock(file.mutex);
        auto result_write = pwrite(file.handle, &value, sizeof(value), offset);
        if (result_write != sizeof(value)) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Invalid write data");
        }
    }
#else
    template <const char* io_name = io_name>
    static reg_value read(const device_file& file, reg_offset offset)
    {
        using namespace std::string_literals;
        const std::lock_guard<std::mutex> lock(file.mutex);
        auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(file.handle), offset, 0, FILE_BEGIN);
        if (result_offset == INVALID_SET_FILE_POINTER) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Invalid set offset");
        }
        reg_value value {};
        auto result_read = ReadFile(reinterpret_cast<HANDLE>(file.handle), &value, sizeof(value), nullptr, nullptr);
        if (result_read == false) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ]  Invalid read data");
        }
        return value;
    }

    template <const char* io_name = io_name>
    static void write(const device_file& file, reg_offset offset, reg_value value)
    {
        using namespace std::string_literals;
        const std::lock_guard<std::mutex> lock(file.mutex);
        auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(file.handle), offset, 0, FILE_BEGIN);
        if (result_offset == INVALID_SET_FILE_POINTER) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Invalid set offset");
        }
        auto result_write = WriteFile(reinterpret_cast<HANDLE>(file.handle), &value, sizeof(value), nullptr, nullptr);
        if (result_write == false) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Invalid write data");
        }
    }
#endif
};

}
