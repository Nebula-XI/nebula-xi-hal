#pragma once

#include "nebulaxi/hal_io.h"

namespace nebulaxi {

enum class dma_channel {
    ch0,
    ch1,
    ch2,
    ch3
};

using dma_buffer = std::vector<uint8_t>;

class hal_dma {
public:
    using unique_ptr = std::unique_ptr<hal_dma>;

    hal_dma() = default;
    hal_dma(const hal_dma&) = delete;
    hal_dma& operator=(const hal_dma&) = delete;
    hal_dma(hal_dma&&) = default;
    hal_dma& operator=(hal_dma&&) = default;
    virtual ~hal_dma() noexcept = default;

    virtual auto read(dma_channel, size_t) const -> dma_buffer = 0;
    virtual void write(dma_channel, const dma_buffer&) const = 0;
};

template <typename dma_type>
hal_dma::unique_ptr make_hal_dma(const std::string& path, const device_info& dev_info)
{
    return std::make_unique<dma_type>(path, dev_info);
}

}
