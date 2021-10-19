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
    auto handle = open(control_name.c_str(), O_RDWR);
    if (handle == EOF)
        throw std::runtime_error("[ XDMA ] Can't open file: \"" + control_name + "\"");
    d_ptr->file_control.handle = handle;

    auto user_name = path + "_user"; // канал управления устройствами на шине `AXI Lite`
    handle = open(user_name.c_str(), O_RDWR);
    if (handle == EOF)
        throw std::runtime_error("[ XDMA ] Can't open file: \"" + user_name + "\"");
    d_ptr->file_user.handle = handle;

    d_ptr->hw_info = hw_info;
    d_ptr->dev_path = path;

    // каналы DMA
    for (auto num : { 0, 1, 2, 3 }) {
        auto name = path + "_c2h_" + std::to_string(num);
        d_ptr->file_c2h.at(num).handle = open(name.c_str(), O_RDONLY | O_TRUNC); // using O_TRUNC to indicate to the driver to flush the data up based on EOP (end-of-packet)

        name = path + "_h2c_" + std::to_string(num);
        d_ptr->file_h2c.at(num).handle = open(name.c_str(), O_WRONLY | O_SYNC);
    }
}

xdma::~xdma()
{
    if (d_ptr == nullptr)
        return;
    close(d_ptr->file_control.handle);
    close(d_ptr->file_user.handle);

    for (auto num : { 0, 1, 2, 3 }) {
        close(d_ptr->file_c2h.at(num).handle);
        close(d_ptr->file_h2c.at(num).handle);
    }
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
// Чтение регистра
// ----------------------------------------------------------------------------
auto xdma::reg_read(xdma_file& file, size_t offset) -> uint32_t
{
    const std::lock_guard<std::mutex> lock(file.mutex);
    uint32_t value {};

    auto result_read = pread(file.handle, &value, sizeof(value), offset);
    if (result_read != sizeof(value))
        throw std::runtime_error("[ XDMA ] Invalid read data");

    return value;
}

// ----------------------------------------------------------------------------
// Запись регистра
// ----------------------------------------------------------------------------
void xdma::reg_write(xdma_file& file, size_t offset, uint32_t value)
{
    const std::lock_guard<std::mutex> lock(file.mutex);

    auto result_write = pwrite(file.handle, &value, sizeof(value), offset);
    if (result_write != sizeof(value))
        throw std::runtime_error("[ XDMA ] Invalid write data");
}

// ----------------------------------------------------------------------------
// Получение данных из DMA канала
// ----------------------------------------------------------------------------
// TODO: сделать каналы в виде enum, чтобы нельзя было передать значение больше четырёх
auto xdma::dma_read(size_t ch_num, size_t len = 4096) -> std::vector<uint8_t>
{
    std::vector<uint8_t> buf {};
    buf.resize(len);
    const std::lock_guard<std::mutex> lock(d_ptr->file_c2h.at(ch_num).mutex);

    auto read_bytes = read(d_ptr->file_c2h.at(ch_num).handle, buf.data(), buf.size());
    buf.resize(read_bytes);
    return buf;
}
