// ----------------------------------------------------------------------------
// Работа с платами под XDMA драйвером
// ----------------------------------------------------------------------------
#pragma once

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "nebulaxi/hal_dma.h"

namespace nebulaxi {

namespace detail {
    struct xdma_data {

        std::array<device_file, 4> file_c2h { -1, {}, -1, {}, -1, {}, -1, {} }; // каналы DMA Card To Host
        std::array<device_file, 4> file_h2c { 1, {}, 1, {} }; // каналы DMA Host To Card
    };
}

class xdma_hal : public hal_dma {
    std::unique_ptr<detail::xdma_data> m_dma { std::make_unique<detail::xdma_data>() };

public:
    static auto
    get_device_paths() -> std::vector<std::pair<std::string const, device_info const>>;

    xdma_hal() = delete;
    xdma_hal(std::string const& path, device_info const& dev_info);
    ~xdma_hal();

    xdma_hal(xdma_hal const&) = delete; // копирование запрещаем
    xdma_hal& operator=(const xdma_hal&) = delete; // копирование запрещаем
    xdma_hal(xdma_hal&&) = default; // перемещение разрешаем
    xdma_hal& operator=(xdma_hal&&) = default; // перемещение разрешаем

    // DMA | Чтение/Запись потоков
    auto read(dma_channel num, size_t len) const -> dma_buffer override;
    auto write(dma_channel num, const dma_buffer& buffer) const -> void override;
};

}
