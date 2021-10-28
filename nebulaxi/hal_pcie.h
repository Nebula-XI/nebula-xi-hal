#pragma once

#include "nebulaxi/hal_io.h"

namespace nebulaxi {

namespace detail {
    struct pcie_data {
        std::string dev_path {};
        device_info dev_info {};
        device_file file_control { -1, {} };
    };
}

class hal_pcie final {
    std::unique_ptr<detail::pcie_data> d_ptr { std::make_unique<detail::pcie_data>() };

#ifdef __linux__
    void open(const std::string& path, const device_info& dev_info)
    {
        using namespace std::string_literals;
        auto control_name = path + "_control";
        auto handle = ::open(control_name.c_str(), O_RDWR);
        if (handle == EOF) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + control_name + "\"");
        }
        d_ptr->file_control.handle = handle;
        d_ptr->dev_info = dev_info;
        d_ptr->dev_path = path;
    }
    void close() const noexcept
    {
        if (d_ptr) {
            ::close(d_ptr->file_control.handle);
        }
    }
#else
    void open(const std::string& path, const device_info& dev_info)
    {
        using namespace std::string_literals;
        auto control_name = path + "\\control";
        auto handle = CreateFileA(control_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + control_name + "\"");
        }
        d_ptr->file_control.handle = static_cast<int>(reinterpret_cast<size_t>(handle));
        d_ptr->dev_info = dev_info;
        d_ptr->dev_path = path;
    }
    void close() const noexcept
    {
        if (d_ptr) {
            CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_control.handle));
        }
    }
#endif

    inline static constexpr char io_name[] { "PCIE" };

public:
    using unique_ptr = std::unique_ptr<hal_pcie>;

    hal_pcie() = delete;
    hal_pcie(const hal_pcie&) = delete;
    hal_pcie& operator=(const hal_pcie&) = delete;
    hal_pcie(hal_pcie&&) = default;
    hal_pcie& operator=(hal_pcie&&) = default;

    hal_pcie(const std::string& path, const device_info& dev_info) { open(path, dev_info); }
    ~hal_pcie() noexcept { close(); }

    auto get_vendor_id() const noexcept { return d_ptr->dev_info.vendor; }
    auto get_device_id() const noexcept { return d_ptr->dev_info.device; }
    auto get_location() const noexcept
    {
        auto value = read(0);
        std::string location {};
        location += std::to_string(d_ptr->dev_info.bus);
        location += std::to_string(d_ptr->dev_info.dev);
        location += std::to_string(d_ptr->dev_info.func);
        return location;
    }
    auto get_device_path() const noexcept { return d_ptr->dev_path; }

    auto read(reg_offset offset) const -> reg_value
    {
        return hal_reg::read<io_name>(d_ptr->file_control, offset);
    }

    void write(reg_offset offset, reg_value value) const
    {
        hal_reg::write<io_name>(d_ptr->file_control, offset, value);
    }
};

hal_pcie::unique_ptr make_hal_pcie(const std::string& path, const device_info& dev_info)
{
    return std::make_unique<hal_pcie>(path, dev_info);
}

}
