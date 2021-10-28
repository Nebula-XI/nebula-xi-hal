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

#ifdef __linux__
    void open(const std::string& path)
    {
        using namespace std::string_literals;
        auto user_name = path + "_user";
        auto handle = ::open(user_name.c_str(), O_RDWR);
        if (handle == EOF) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + user_name + "\"");
        }
        d_ptr->file_user.handle = handle;
    }
    void close() const noexcept
    {
        if (d_ptr) {
            ::close(d_ptr->file_user.handle);
        }
    }
#else
    void open(const std::string& path)
    {
        using namespace std::string_literals;
        auto user_name = path + "\\user";
        auto handle = CreateFileA(user_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + user_name + "\"");
        }
        d_ptr->file_user.handle = static_cast<int>(reinterpret_cast<size_t>(handle));
    }
    void close() const noexcept
    {
        if (d_ptr) {
            CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_user.handle));
        }
    }
#endif

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

    auto read(reg_offset offset) const -> reg_value
    {
        return hal_reg::read<io_name>(d_ptr->file_user, offset);
    }
    void write(reg_offset offset, reg_value value) const
    {
        hal_reg::write<io_name>(d_ptr->file_user, offset, value);
    };
};

hal_axi::unique_ptr make_hal_axi(const std::string& path)
{
    return std::make_unique<hal_axi>(path);
}

}
