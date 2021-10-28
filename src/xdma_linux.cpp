// ----------------------------------------------------------------------------
// Вариант для Linux
// ----------------------------------------------------------------------------
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <xdma_hal.hpp>

using namespace nebulaxi;

xdma_hal::xdma_hal(std::string const& path, device_info const& dev_info)
{
    for (auto num : { 0, 1, 2, 3 }) {
        auto name = path + "_c2h_" + std::to_string(num);
        m_dma->file_c2h.at(num).handle = open(name.c_str(), O_RDONLY | O_TRUNC); // using O_TRUNC to indicate to the driver to flush the data up based on EOP (end-of-packet)

        name = path + "_h2c_" + std::to_string(num);
        m_dma->file_h2c.at(num).handle = open(name.c_str(), O_WRONLY | O_SYNC);
    }
}

xdma_hal::~xdma_hal()
{
    for (auto num : { 0, 1, 2, 3 }) {
        close(m_dma->file_c2h.at(num).handle);
        close(m_dma->file_h2c.at(num).handle);
    }
};

// ----------------------------------------------------------------------------
// Поиск всех присутствующих плат
// ----------------------------------------------------------------------------
auto xdma_hal::get_device_paths() -> std::vector<std::pair<std::string const, device_info const>>
{
    std::vector<std::pair<std::string const, device_info const>> dev_paths {};

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

        dev_paths.emplace_back(dev_name, device_info { xdma_ioc_info.vendor, xdma_ioc_info.device, xdma_ioc_info.bus, xdma_ioc_info.dev, xdma_ioc_info.func });
    }

    if (dev_paths.empty())
        throw std::runtime_error("No PCIe boards");
    else
        return dev_paths;
}

// ----------------------------------------------------------------------------
// Получение данных из DMA канала
// ----------------------------------------------------------------------------
// TODO: сделать каналы в виде enum, чтобы нельзя было передать значение больше четырёх
auto xdma_hal::read(dma_channel channel, size_t size = 4096) const -> dma_buffer
{
    auto ch { static_cast<std::size_t>(channel) };
    dma_buffer buf {};
    buf.resize(size);
    const std::lock_guard<std::mutex> lock(m_dma->file_c2h.at(ch).mutex);

    auto read_bytes = ::read(m_dma->file_c2h.at(ch).handle, buf.data(), buf.size());
    buf.resize(read_bytes);
    return buf;
}

auto xdma_hal::write(dma_channel channel, const dma_buffer& buffer) const -> void
{
    // TODO:
}
