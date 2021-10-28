#pragma once

#include "nebulaxi/hal_dma.h"

namespace nebulaxi {

namespace detail {
    struct xdma_data {
        std::array<device_file, 4> file_c2h { -1, {}, -1, {}, -1, {}, -1, {} }; // каналы DMA Card To Host
        std::array<device_file, 4> file_h2c { 1, {}, 1, {} }; // каналы DMA Host To Card
    };
}

using device_path_list = std::vector<std::pair<const std::string, const device_info>>;

class hal_xdma final : public hal_dma {
    std::unique_ptr<detail::xdma_data> m_dma { std::make_unique<detail::xdma_data>() };

    inline static constexpr char io_name[] { "XMDA" };

public:
    using unique_ptr = std::unique_ptr<hal_dma>;

    hal_xdma() = delete;
    hal_xdma(const hal_xdma&) = delete;
    hal_xdma& operator=(const hal_xdma&) = delete;
    hal_xdma(hal_xdma&&) = default;
    hal_xdma& operator=(hal_xdma&&) = default;

#ifdef __linux__
    static auto get_device_paths() -> device_path_list
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

    hal_xdma(const std::string& path, const device_info& dev_info)
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
    ~hal_xdma() noexcept
    {
        for (auto num : { 0, 1, 2, 3 }) {
            close(m_dma->file_c2h.at(num).handle);
            close(m_dma->file_h2c.at(num).handle);
        }
    }

    auto read(dma_channel channel, size_t size = 4096) const -> dma_buffer override
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

    auto write(dma_channel num, const dma_buffer& buffer) const -> void override
    {
        // TODO:
    }
#else
    static auto get_device_paths() -> device_path_list
    {
        device_path_list dev_paths {};

        auto dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_XDMA, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (dev_info == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("No PCIe boards");
        }

        SP_DEVICE_INTERFACE_DATA dev_interface {};
        dev_interface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        DWORD index {};
        while (SetupDiEnumDeviceInterfaces(dev_info, nullptr, &GUID_DEVINTERFACE_XDMA, index++, &dev_interface)) {
            DWORD detail_len {};
            SetupDiGetDeviceInterfaceDetailA(dev_info, &dev_interface, nullptr, 0, &detail_len, nullptr);
            std::vector<uint8_t> buffer(detail_len);
            auto dev_detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(buffer.data());
            dev_detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
            SP_DEVINFO_DATA dev_data {};
            dev_data.cbSize = sizeof(SP_DEVINFO_DATA);
            SetupDiGetDeviceInterfaceDetailA(dev_info, &dev_interface, dev_detail, detail_len, nullptr, &dev_data);

            // получим расположение платы на шине
            uint32_t bus, addr, slot;
            SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_data, SPDRP_BUSNUMBER, nullptr, PBYTE(&bus), sizeof(bus), nullptr);
            SetupDiGetDeviceRegistryPropertyA(dev_info, &dev_data, SPDRP_ADDRESS, nullptr, PBYTE(&addr), sizeof(addr), nullptr);
            slot = (addr >> 16) & 0xFFFF;

            // получим vendor и device id
            std::string path { dev_detail->DevicePath };
            auto ven_id = uint32_t(std::stoi(path.substr(path.find("ven_") + 4, 4).data(), 0, 16));
            auto dev_id = uint32_t(std::stoi(path.substr(path.find("dev_") + 4, 4).data(), 0, 16));

            dev_paths.emplace_back(dev_detail->DevicePath, device_info { ven_id, dev_id, bus, addr, slot });
        }
        SetupDiDestroyDeviceInfoList(dev_info);

        if (dev_paths.empty()) {
            throw std::runtime_error("No PCIe boards");
        } else {
            return dev_paths;
        }
    }

    hal_xdma(const std::string& path, const device_info& dev_info)
    {
        // FIXME: if open error?
        for (auto num : { 0, 1, 2, 3 }) {
            auto name = path + "\\c2h_" + std::to_string(num);
            auto handle = CreateFileA(name.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (handle != INVALID_HANDLE_VALUE) {
                m_dma->file_c2h.at(num).handle = static_cast<int>(reinterpret_cast<size_t>(handle));
            }
            name = path + "\\h2c_" + std::to_string(num);
            handle = CreateFileA(name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (handle != INVALID_HANDLE_VALUE) {
                m_dma->file_h2c.at(num).handle = static_cast<int>(reinterpret_cast<size_t>(handle));
            }
        }
    }
    ~hal_xdma() noexcept
    {
        for (auto num : { 0, 1, 2, 3 }) {
            CloseHandle(reinterpret_cast<HANDLE>(m_dma->file_c2h.at(num).handle));
            CloseHandle(reinterpret_cast<HANDLE>(m_dma->file_h2c.at(num).handle));
        }
    }

    auto read(dma_channel channel, size_t size = 4096) const -> dma_buffer override
    {
        using namespace std::string_literals;
        auto _channel { static_cast<std::size_t>(channel) };
        dma_buffer buffer {};
        buffer.resize(size);
        const std::lock_guard<std::mutex> lock(m_dma->file_c2h.at(_channel).mutex);
        DWORD read_bytes {};
        auto result_read = ReadFile(
            reinterpret_cast<HANDLE>(m_dma->file_c2h.at(_channel).handle), buffer.data(), buffer.size(), &read_bytes, nullptr);
        if (result_read == false) {
            throw std::runtime_error("[ "s + std::string { io_name } + " ] Invalid read data");
        }
        buffer.resize(read_bytes);
        return buffer;
    }
    auto write(dma_channel num, const dma_buffer& buffer) const -> void override
    {
        // TODO:
    }
#endif
};

// FIXME:
#ifndef __linux__
DEFINE_GUID(GUID_DEVINTERFACE_XDMA, 0x74c7e4a9, 0x6d5d, 0x4a70, 0xbc, 0x0d, 0x20, 0x69, 0x1d, 0xff, 0x9e, 0x9d);
#endif

}
