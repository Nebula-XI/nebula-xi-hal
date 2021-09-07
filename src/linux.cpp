// ----------------------------------------------------------------------------
// Вариант для Linux
// ----------------------------------------------------------------------------
#include "xdma.hpp"

#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

xdma::xdma(std::string const& path, xdma_additional_info const& hw_info)
{
    auto control_name = path + "_control"; // канал управления `DMA/Bridge PCI Express`
    d_ptr->handle_control = open(control_name.c_str(), O_RDWR);
    if (d_ptr->handle_control == EOF)
        throw std::runtime_error("[ XDMA ] Can't open file: \"" + control_name + "\"");

    auto user_name = path + "_user"; // канал управления устройствами на шине `AXI Lite`
    d_ptr->handle_user = open(user_name.c_str(), O_RDWR);
    if (d_ptr->handle_user == EOF)
        throw std::runtime_error("[ XDMA ] Can't open file: \"" + user_name + "\"");

    d_ptr->hw_info = hw_info;
    d_ptr->dev_path = path;
}

xdma::~xdma()
{
    if (d_ptr == nullptr)
        return;
    close(d_ptr->handle_control);
    close(d_ptr->handle_user);
};

// ----------------------------------------------------------------------------
// Поиск всех присутствующих плат
// ----------------------------------------------------------------------------
auto xdma::get_device_paths() -> std::vector<std::pair<std::string const, xdma_additional_info const>>
{
    std::vector<std::pair<std::string const, xdma_additional_info const>> dev_paths {};

    // перебор плат
    size_t index {};
    while (true) {
        auto dev_name = "/dev/xdma" + std::to_string(index++);
        auto handle = open((dev_name + "_control").data(), O_RDWR);
        if (handle == EOF)
            break;

        // получим доп. информацию о плате
        struct xdma_ioc_base {
            unsigned int magic { 0x586C0C6C }; // XL OpenCL X->58(ASCII), L->6C(ASCII), O->0 C->C L->6C(ASCII)
            unsigned int command;
        };
        struct xdma_ioc_info {
            struct xdma_ioc_base base;
            unsigned short vendor;
            unsigned short device;
            unsigned short subsystem_vendor;
            unsigned short subsystem_device;
            unsigned int dma_engine_version;
            unsigned int driver_version;
            unsigned long long feature_id;
            unsigned short domain;
            unsigned char bus;
            unsigned char dev;
            unsigned char func;
        } xdma_ioc_info {};

        auto result = ioctl(handle, _IOWR('x', 1 /*XDMA_IOC_INFO*/, struct xdma_ioc_info), &xdma_ioc_info);
        if (result != 0)
            throw std::runtime_error("[ IOCTL ] Error get information");

        dev_paths.emplace_back(dev_name, xdma_additional_info { xdma_ioc_info.vendor, xdma_ioc_info.device, xdma_ioc_info.bus, xdma_ioc_info.dev, xdma_ioc_info.func });
    }

    return dev_paths;
}

// ----------------------------------------------------------------------------
// Чтение из PCI Express блока
// ----------------------------------------------------------------------------
auto xdma::pcie_reg_read(size_t offset) -> uint32_t
{
    uint32_t value {};

    if (pread(d_ptr->handle_control, &value, sizeof(value), offset) != sizeof(value))
        throw std::runtime_error("[ PCIe ] Invalid read data");

    return value;
}

// ----------------------------------------------------------------------------
// Запись в PCI Express блок
// ----------------------------------------------------------------------------
auto xdma::pcie_reg_write(size_t offset, uint32_t value) -> void
{
    if (pwrite(d_ptr->handle_control, &value, sizeof(value), offset) != sizeof(value))
        throw std::runtime_error("[ PCIe ] Invalid write data");
}

// ----------------------------------------------------------------------------
// Чтение на шине AXI
// ----------------------------------------------------------------------------
auto xdma::axi_reg_read(size_t offset) -> uint32_t
{
    uint32_t value {};

    if (pread(d_ptr->handle_user, &value, sizeof(value), offset) != sizeof(value))
        throw std::runtime_error("[ AXI ] Invalid read data");

    return value;
}

// ----------------------------------------------------------------------------
// Запись на шине AXI
// ----------------------------------------------------------------------------
auto xdma::axi_reg_write(size_t offset, uint32_t value) -> void
{
    if (pwrite(d_ptr->handle_user, &value, sizeof(value), offset) != sizeof(value))
        throw std::runtime_error("[ AXI ] Invalid write data");
}
