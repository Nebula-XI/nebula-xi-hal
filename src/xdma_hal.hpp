// ----------------------------------------------------------------------------
// Работа с платами под XDMA драйвером
// ----------------------------------------------------------------------------
#pragma once
#include <array>
#include <cstdio>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

struct xdma_additional_info {
    uint32_t vendor;
    uint32_t device;
    uint32_t bus;
    uint32_t dev;
    uint32_t func;
};

struct xdma_file {
    int handle;
    std::mutex mutex;
};

struct xdma_data {
    std::string dev_path {}; // путь к плате
    xdma_additional_info hw_info {}; // информация о плате

    xdma_file file_control { -1, {} }; // канал управления `DMA/Bridge PCI Express`
    xdma_file file_user { -1, {} }; // канал управления устройствами на шине `AXI Lite`
    std::array<xdma_file, 4> file_c2h { -1, {}, -1, {}, -1, {}, -1, {} }; // каналы DMA Card To Host
    std::array<xdma_file, 4> file_h2c { 1, {}, 1, {} }; // каналы DMA Host To Card
};

class xdma_hal {
    std::unique_ptr<xdma_data> d_ptr = { std::make_unique<xdma_data>() };

    auto reg_read(xdma_file& file, size_t offset) -> uint32_t;
    void reg_write(xdma_file& file, size_t offset, uint32_t value);

public:
    static auto
    get_device_paths() -> std::vector<std::pair<std::string const, xdma_additional_info const>>;

    xdma_hal() = delete;
    xdma_hal(std::string const& path, xdma_additional_info const& hw_info);
    ~xdma_hal();

    xdma_hal(xdma_hal const&) = delete; // копирование запрещаем
    xdma_hal& operator=(const xdma_hal&) = delete; // копирование запрещаем
    xdma_hal(xdma_hal&&) = default; // перемещение разрешаем
    xdma_hal& operator=(xdma_hal&&) = default; // перемещение разрешаем

    auto get_vendor_id() { return d_ptr->hw_info.vendor; }
    auto get_device_id() { return d_ptr->hw_info.device; }
    auto const get_pci_location() { return std::to_string(d_ptr->hw_info.bus) + std::to_string(d_ptr->hw_info.dev) + std::to_string(d_ptr->hw_info.func); }
    auto const get_device_path() { return d_ptr->dev_path; }

    // PCI Express | Чтение/Запись регистров
    auto pcie_reg_read(size_t offset) { return reg_read(d_ptr->file_control, offset); };
    void pcie_reg_write(size_t offset, uint32_t value) { reg_write(d_ptr->file_control, offset, value); };

    // AXI | Чтение/Запись регистров
    auto axi_reg_read(size_t offset) { return reg_read(d_ptr->file_user, offset); };
    void axi_reg_write(size_t offset, uint32_t value) { reg_write(d_ptr->file_user, offset, value); };

    // DMA | Чтение/Запись потоков
    auto dma_read(size_t num, size_t len) -> std::vector<uint8_t>;
    auto dma_read(size_t num, std::vector<uint8_t>& buffer) -> void;

    // Чтение CFGROM
    auto get_cfgrom() -> std::vector<uint8_t>
    {
        std::vector<uint8_t> cfgrom {};
        bool eof{};
        while (!eof) {
            auto value = axi_reg_read(cfgrom.size());
            if (value == 0)
                break;
            for (auto it : { 0, 8, 16, 24 }) {
                auto byte = (value & (0xFF << it)) >> it;
                if (byte != 0)
                    cfgrom.push_back(byte);
                else
                    eof = true;
            }
        }
        return cfgrom;
    }
};
