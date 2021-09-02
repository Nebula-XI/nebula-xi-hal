// ----------------------------------------------------------------------------
// Работа с платами под XDMA драйвером
// ----------------------------------------------------------------------------
#pragma once
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

struct xdma_additional_info {
    uint32_t vendor;
    uint32_t device;
    uint32_t bus;
    uint32_t dev;
    uint32_t func;
};

struct xdma_data {
    int handle_control { -1 }; // канал управления `DMA/Bridge PCI Express`
    int handle_user { -1 }; // канал управления устройствами на шине `AXI Lite`
    xdma_additional_info hw_info {}; // информация о плате
};

class xdma {
    std::unique_ptr<xdma_data> d_ptr = { std::make_unique<xdma_data>() };

public:
    static auto get_device_paths() -> std::vector<std::pair<std::string const, xdma_additional_info const>>;

    xdma() = delete;
    xdma(std::string const& path, xdma_additional_info const& hw_info);
    ~xdma();

    xdma(xdma const&) = delete; // копирование запрещаем
    xdma& operator=(const xdma&) = delete; // копирование запрещаем
    xdma(xdma&&) = default; // перемещение разрешаем
    xdma& operator=(xdma&&) = default; // перемещение разрешаем

    auto pcie_reg_read(size_t offset) -> uint32_t;
    auto pcie_reg_write(size_t offset, uint32_t value) -> void;

    auto axi_reg_read(size_t offset) -> uint32_t;
    auto axi_reg_write(size_t offset, uint32_t value) -> void;

    auto get_vendor_id() { return d_ptr->hw_info.vendor; }
    auto get_device_id() { return d_ptr->hw_info.device; }
    auto const get_pci_location() { return std::to_string(d_ptr->hw_info.bus) + "." + std::to_string(d_ptr->hw_info.dev) + "." + std::to_string(d_ptr->hw_info.func); }
};
