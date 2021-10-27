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

public:
    using unique_ptr = std::unique_ptr<hal_pcie>;

    hal_pcie() = delete;
    hal_pcie(const hal_pcie&) = delete;
    hal_pcie& operator=(const hal_pcie&) = delete;
    hal_pcie(hal_pcie&&) = default;
    hal_pcie& operator=(hal_pcie&&) = default;
    hal_pcie(const std::string& path, const device_info& dev_info) { open(path, dev_info); }
    ~hal_pcie() noexcept { close(); }
    void open(const std::string& path, const device_info& dev_info)
    {
#ifdef __linux__
        auto control_name = path + "_control";
        auto handle = ::open(control_name.c_str(), O_RDWR);
        if (handle == EOF)
            throw std::runtime_error("[ XDMA ] Can't open file: \"" + control_name + "\"");
        d_ptr->file_control.handle = handle;
        d_ptr->dev_info = dev_info;
        d_ptr->dev_path = path;
#else
        auto control_name = path + "\\control";
        auto handle = CreateFileA(control_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("[ XDMA ] Can't open file: \"" + control_name + "\"");
        d_ptr->file_control.handle = static_cast<int>(reinterpret_cast<size_t>(handle));
        d_ptr->dev_info = dev_info;
        d_ptr->dev_path = path;
#endif
    }
    void close() const noexcept
    {
#ifdef __linux__
        ::close(d_ptr->file_control.handle);
#else
        CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_control.handle));
#endif
    }
    auto get_vendor_id() const noexcept { return d_ptr->dev_info.vendor; }
    auto get_device_id() const noexcept { return d_ptr->dev_info.device; }
    auto get_location() const noexcept
    {
        auto value = reg_read(0);
        std::string location {};
        location += std::to_string(d_ptr->dev_info.bus);
        location += std::to_string(d_ptr->dev_info.dev);
        location += std::to_string(d_ptr->dev_info.func);
        return location;
    }
    auto get_device_path() const noexcept { return d_ptr->dev_path; }
    auto reg_read(reg_offset offset) const -> reg_value { return io::reg_read(d_ptr->file_control, offset); }
    void reg_write(reg_offset offset, reg_value value) const { io::reg_write(d_ptr->file_control, offset, value); }
};

auto make_hal_pcie(const std::string& path, const device_info& dev_info) { return std::make_unique<hal_pcie>(path, dev_info); }

}
