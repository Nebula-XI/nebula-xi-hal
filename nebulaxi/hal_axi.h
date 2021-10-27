#pragma once

#include "nebulaxi/hal_io.h"

namespace nebulaxi {

namespace detail {
    struct axi_data {
        device_file file_user { -1, {} };
    };
}

class hal_axi final {
    std::unique_ptr<detail::axi_data> d_ptr { std::make_unique<detail::axi_data>() };

    void open(const std::string& path)
    {
#ifdef __linux__
        auto user_name = path + "_user";
        auto handle = ::open(user_name.c_str(), O_RDWR);
        if (handle == EOF)
            throw std::runtime_error("[ XDMA ] Can't open file: \"" + user_name + "\"");
        d_ptr->file_user.handle = handle;
#else
        auto user_name = path + "\\user";
        auto handle = CreateFileA(user_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle == INVALID_HANDLE_VALUE)
            throw std::runtime_error("[ XDMA ] Can't open file: \"" + user_name + "\"");
        d_ptr->file_user.handle = static_cast<int>(reinterpret_cast<size_t>(handle));
#endif
    }
    void close() const noexcept
    {
#ifdef __linux__
        ::close(d_ptr->file_user.handle);
#else
        CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_user.handle));
#endif
    }

public:
    using unique_ptr = std::unique_ptr<hal_axi>;

    hal_axi() = delete;
    hal_axi(const hal_axi&) = delete;
    hal_axi& operator=(const hal_axi&) = delete;
    hal_axi(hal_axi&&) = default;
    hal_axi& operator=(hal_axi&&) = default;
    hal_axi(const std::string& path) { open(path); }
    ~hal_axi() noexcept { close(); }
    auto reg_read(reg_offset offset) const -> reg_value { return io::reg_read(d_ptr->file_user, offset); }
    void reg_write(reg_offset offset, reg_value value) const { io::reg_write(d_ptr->file_user, offset, value); };
};

auto make_hal_axi(const std::string& path) { return std::make_unique<hal_axi>(path); }

}
