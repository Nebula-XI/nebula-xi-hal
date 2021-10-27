#pragma once

#ifdef __linux__
#include <sys/ioctl.h>
#include <unistd.h>
#else
#include <windows.h>
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

using reg_offset = std::size_t;
using reg_value = uint32_t;

namespace io {
#ifdef __linux__
    reg_value reg_read(const device_file& file, reg_offset offset)
    {
        const std::lock_guard<std::mutex> lock(file.mutex);
        reg_value value {};
        auto result_read = pread(file.handle, &value, sizeof(value), offset);
        if (result_read != sizeof(value))
            throw std::runtime_error("[ XDMA ] Invalid read data");
        return value;
    }
    void reg_write(const device_file& file, reg_offset offset, reg_value value)
    {
        const std::lock_guard<std::mutex> lock(file.mutex);
        auto result_write = pwrite(file.handle, &value, sizeof(value), offset);
        if (result_write != sizeof(value))
            throw std::runtime_error("[ XDMA ] Invalid write data");
    }
#else
    reg_value reg_read(const device_file& file, reg_offset offset)
    {
        const std::lock_guard<std::mutex> lock(file.mutex);
        auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(file.handle), offset, 0, FILE_BEGIN);
        if (result_offset == INVALID_SET_FILE_POINTER)
            throw std::runtime_error("[ XDMA ] Invalid set offset");
        reg_value value {};
        auto result_read = ReadFile(reinterpret_cast<HANDLE>(file.handle), &value, sizeof(value), nullptr, nullptr);
        if (result_read == false)
            throw std::runtime_error("[ XDMA ] Invalid read data");
        return value;
    }
    void reg_write(const device_file& file, reg_offset offset, reg_value value)
    {
        const std::lock_guard<std::mutex> lock(file.mutex);
        auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(file.handle), offset, 0, FILE_BEGIN);
        if (result_offset == INVALID_SET_FILE_POINTER)
            throw std::runtime_error("[ XDMA ] Invalid set offset");
        auto result_write = WriteFile(reinterpret_cast<HANDLE>(file.handle), &value, sizeof(value), nullptr, nullptr);
        if (result_write == false)
            throw std::runtime_error("[ XDMA ] Invalid write data");
    }
#endif
}

}
