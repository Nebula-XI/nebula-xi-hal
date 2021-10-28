#pragma once

#include "nebulaxi/hal_dma.h"

namespace nebulaxi {

namespace detail {
    struct xdma_data {
        std::array<device_file, 4> file_c2h { -1, {}, -1, {}, -1, {}, -1, {} }; // DMA Card To Host
        std::array<device_file, 4> file_h2c { 1, {}, 1, {} }; // DMA Host To Card
    };
}

using device_path_list = std::vector<std::pair<const std::string, const device_info>>;

class hal_xdma final : public hal_dma {
    std::unique_ptr<detail::xdma_data> m_dma { std::make_unique<detail::xdma_data>() };

    inline static constexpr char io_name[] { "XMDA" };

public:
    using unique_ptr = std::unique_ptr<hal_xdma>;

    hal_xdma() = delete;
    hal_xdma(const hal_xdma&) = delete;
    hal_xdma& operator=(const hal_xdma&) = delete;
    hal_xdma(hal_xdma&&) = default;
    hal_xdma& operator=(hal_xdma&&) = default;

    static auto get_device_paths() -> device_path_list;

    hal_xdma(const std::string&, const device_info&);
    ~hal_xdma() noexcept;

    auto read(dma_channel, size_t size = 4096) const -> dma_buffer override;
    auto write(dma_channel, const dma_buffer&) const -> void override;
};

hal_dma::unique_ptr make_hal_xdma(const std::string& path, const device_info& dev_info)
{
    return make_hal_dma<hal_xdma>(path, dev_info);
}

}
