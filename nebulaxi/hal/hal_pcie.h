#pragma once

#include "nebulaxi/hal/hal_io.h"

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

    void open(const std::string&, const device_info&);
    void close() const noexcept;

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
    auto get_device_path() const noexcept { return d_ptr->dev_path; }
    auto get_location() const noexcept -> std::string;

    auto read(reg_offset) const -> reg_value;
    void write(reg_offset, reg_value) const;
};

inline hal_pcie::unique_ptr make_hal_pcie(const std::string& path, const device_info& dev_info)
{
    return std::make_unique<hal_pcie>(path, dev_info);
}

}
