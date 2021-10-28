// ----------------------------------------------------------------------------
// Вариант для Windows
// ----------------------------------------------------------------------------
// clang-format off
#include <windows.h>
#include <initguid.h>
#include <setupapi.h>
// clang-format on
#include <xdma_hal.hpp>

using namespace nebulaxi;

DEFINE_GUID(GUID_DEVINTERFACE_XDMA, 0x74c7e4a9, 0x6d5d, 0x4a70, 0xbc, 0x0d, 0x20, 0x69, 0x1d, 0xff, 0x9e, 0x9d);

xdma_hal::xdma_hal(std::string const& path, device_info const& dev_info)
{
    for (auto num : { 0, 1, 2, 3 }) {
        auto name = path + "\\c2h_" + std::to_string(num);
        auto handle = CreateFileA(name.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle != INVALID_HANDLE_VALUE)
            m_dma->file_c2h.at(num).handle = static_cast<int>(reinterpret_cast<size_t>(handle));

        name = path + "\\h2c_" + std::to_string(num);
        handle = CreateFileA(name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle != INVALID_HANDLE_VALUE)
            m_dma->file_h2c.at(num).handle = static_cast<int>(reinterpret_cast<size_t>(handle));
    }
}

xdma_hal::~xdma_hal()
{
    for (auto num : { 0, 1, 2, 3 }) {
        CloseHandle(reinterpret_cast<HANDLE>(m_dma->file_c2h.at(num).handle));
        CloseHandle(reinterpret_cast<HANDLE>(m_dma->file_h2c.at(num).handle));
    }
};

// ----------------------------------------------------------------------------
// Поиск всех присутствующих плат
// ----------------------------------------------------------------------------
auto xdma_hal::get_device_paths() -> std::vector<std::pair<std::string const, device_info const>>
{
    std::vector<std::pair<std::string const, device_info const>> dev_paths {};

    auto dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_XDMA, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE)
        throw std::runtime_error("No PCIe boards");

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

    if (dev_paths.empty())
        throw std::runtime_error("No PCIe boards");
    else
        return dev_paths;
}

// ----------------------------------------------------------------------------
// Получение данных из DMA канала
// ----------------------------------------------------------------------------
// TODO: сделать каналы в виде enum, чтобы нельзя было передать значение больше четырёх
auto xdma_hal::read(dma_channel channel, size_t len = 4096) const -> dma_buffer
{
    auto ch { static_cast<std::size_t>(channel) };
    dma_buffer buf {};
    buf.resize(len);
    const std::lock_guard<std::mutex> lock(m_dma->file_c2h.at(ch).mutex);

    DWORD read_bytes {};
    auto result_read = ReadFile(reinterpret_cast<HANDLE>(m_dma->file_c2h.at(ch).handle), buf.data(), buf.size(), &read_bytes, nullptr);
    if (result_read == false)
        throw std::runtime_error("[ C2H ] Invalid read data");

    buf.resize(read_bytes);
    return buf;
}

auto xdma_hal::write(dma_channel channel, const dma_buffer& buffer) const -> void
{
    // TODO:
}
