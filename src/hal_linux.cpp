#include "nebulaxi/hal/hal_axi.h"
#include "nebulaxi/hal/hal_pcie.h"
#include "nebulaxi/hal/hal_xdma.h"

using namespace nebulaxi;

void hal_pcie::open(const std::string& path, const device_info& dev_info)
{
    using namespace std::string_literals;
    auto control_name = path + "_control";
    auto handle = ::open(control_name.c_str(), O_RDWR);
    if (handle == EOF) {
        throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + control_name + "\"");
    }
    d_ptr->file_control.handle = handle;
    d_ptr->dev_info = dev_info;
    d_ptr->dev_path = path;
}

void hal_pcie::close() const noexcept
{
    if (d_ptr) {
        ::close(d_ptr->file_control.handle);
    }
}

void hal_axi::open(const std::string& path)
{
    using namespace std::string_literals;
    auto user_name = path + "_user";
    auto handle = ::open(user_name.c_str(), O_RDWR);
    if (handle == EOF) {
        throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + user_name + "\"");
    }
    d_ptr->file_user.handle = handle;
}

void hal_axi::close() const noexcept
{
    if (d_ptr) {
        ::close(d_ptr->file_user.handle);
    }
}

device_path_list get_device_paths()
{
    device_path_list dev_paths {};

    // перебор плат
    size_t index {};
    while (true) {
        auto dev_name = "/dev/xdma" + std::to_string(index++);
        auto handle = open((dev_name + "_control").data(), O_RDWR);
        if (handle == EOF) {
            break;
        }
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

        dev_paths.emplace_back(dev_name,
            device_info { xdma_ioc_info.vendor, xdma_ioc_info.device, xdma_ioc_info.bus, xdma_ioc_info.dev, xdma_ioc_info.func });
    }
    if (dev_paths.empty()) {
        throw std::runtime_error("No PCIe boards");
    } else {
        return dev_paths;
    }
}

hal_xdma::hal_xdma(const std::string& path, const device_info& dev_info)
{
    // FIXME: if open error?
    for (auto num : { 0, 1, 2, 3 }) {
        auto name = path + "_c2h_" + std::to_string(num);
        // using O_TRUNC to indicate to the driver to flush the data up based on EOP (end-of-packet)
        m_dma->file_c2h.at(num).handle = open(name.c_str(), O_RDONLY | O_TRUNC);
        name = path + "_h2c_" + std::to_string(num);
        m_dma->file_h2c.at(num).handle = open(name.c_str(), O_WRONLY | O_SYNC);
    }
}

hal_xdma::~hal_xdma() noexcept
{
    for (auto num : { 0, 1, 2, 3 }) {
        close(m_dma->file_c2h.at(num).handle);
        close(m_dma->file_h2c.at(num).handle);
    }
}

auto hal_xdma::read(dma_channel channel, size_t size) const -> dma_buffer
{
    auto _channel { static_cast<std::size_t>(channel) };
    dma_buffer buffer {};
    buffer.resize(size);
    const std::lock_guard<std::mutex> lock(m_dma->file_c2h.at(_channel).mutex);
    // FIXME: if read error?
    auto read_bytes = ::read(m_dma->file_c2h.at(_channel).handle, buffer.data(), buffer.size());
    buffer.resize(read_bytes);
    return buffer;
}

auto hal_xdma::write(dma_channel num, const dma_buffer& buffer) const -> void
{
    // TODO:
}
