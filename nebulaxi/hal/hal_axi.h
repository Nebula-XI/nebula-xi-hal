#pragma once

#include "nebulaxi/hal/hal_io.h"

namespace nebulaxi {

namespace detail {
    struct axi_data {
        device_file file_user { -1, {} };
    };
}

class hal_axi final {
    std::unique_ptr<detail::axi_data> d_ptr { std::make_unique<detail::axi_data>() };

    void open(const std::string&);
    void close() const noexcept;

    inline static constexpr char io_name[] { "AXI" };

public:
    using unique_ptr = std::unique_ptr<hal_axi>;

    hal_axi() = delete;
    hal_axi(const hal_axi&) = delete;
    hal_axi& operator=(const hal_axi&) = delete;
    hal_axi(hal_axi&&) = default;
    hal_axi& operator=(hal_axi&&) = default;

    hal_axi(const std::string& path) { open(path); }
    ~hal_axi() noexcept { close(); }

    auto read(reg_offset) const -> reg_value;
    void write(reg_offset, reg_value) const;
};

inline hal_axi::unique_ptr make_hal_axi(const std::string& path)
{
    return std::make_unique<hal_axi>(path);
}

}
