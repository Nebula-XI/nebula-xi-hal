// ----------------------------------------------------------------------------
// Вариант для Windows
// ----------------------------------------------------------------------------
#include "xdma.hpp"
// clang-format off
#include <windows.h>
#include <initguid.h>
#include <setupapi.h>
// clang-format on

DEFINE_GUID(GUID_DEVINTERFACE_XDMA, 0x74c7e4a9, 0x6d5d, 0x4a70, 0xbc, 0x0d, 0x20, 0x69, 0x1d, 0xff, 0x9e, 0x9d);

xdma::xdma(std::string const& path, xdma_additional_info const& hw_info)
{
    auto control_name = path + "\\control"; // канал управления `DMA/Bridge PCI Express`
    auto handle = CreateFileA(control_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("[ XDMA ] Can't open file: \"" + control_name + "\"");
    d_ptr->file_control.handle = static_cast<int>(reinterpret_cast<size_t>(handle));

    auto user_name = path + "\\user"; // канал управления устройствами на шине `AXI Lite`
    handle = CreateFileA(user_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("[ XDMA ] Can't open file: \"" + user_name + "\"");
    d_ptr->file_user.handle = static_cast<int>(reinterpret_cast<size_t>(handle));

    d_ptr->hw_info = hw_info;
    d_ptr->dev_path = path;

    // каналы DMA
    for (auto num : { 0, 1, 2, 3 }) {
        auto name = path + "\\c2h_" + std::to_string(num);
        auto handle = CreateFileA(name.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle != INVALID_HANDLE_VALUE)
            d_ptr->file_c2h.at(num).handle = static_cast<int>(reinterpret_cast<size_t>(handle));

        name = path + "\\h2c_" + std::to_string(num);
        handle = CreateFileA(name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (handle != INVALID_HANDLE_VALUE)
            d_ptr->file_h2c.at(num).handle = static_cast<int>(reinterpret_cast<size_t>(handle));
    }
}

xdma::~xdma()
{
    if (d_ptr == nullptr)
        return;
    CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_control.handle));
    CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_user.handle));

    for (auto num : { 0, 1, 2, 3 }) {
        CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_c2h.at(num).handle));
        CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_h2c.at(num).handle));
    }
};

// ----------------------------------------------------------------------------
// Поиск всех присутствующих плат
// ----------------------------------------------------------------------------
auto xdma::get_device_paths() -> std::vector<std::pair<std::string const, xdma_additional_info const>>
{
    std::vector<std::pair<std::string const, xdma_additional_info const>> dev_paths {};

    auto dev_info = SetupDiGetClassDevsA(&GUID_DEVINTERFACE_XDMA, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE)
        return dev_paths;

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

        dev_paths.emplace_back(dev_detail->DevicePath, xdma_additional_info { ven_id, dev_id, bus, addr, slot });
    }
    SetupDiDestroyDeviceInfoList(dev_info);

    return dev_paths;
}

// ----------------------------------------------------------------------------
// Чтение регистра
// ----------------------------------------------------------------------------
auto xdma::reg_read(xdma_file& file, size_t offset) -> uint32_t
{
    const std::lock_guard<std::mutex> lock(file.mutex);

    auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(file.handle), offset, 0, FILE_BEGIN);
    if (result_offset == INVALID_SET_FILE_POINTER)
        throw std::runtime_error("[ XDMA ] Invalid set offset");

    uint32_t value {};
    auto result_read = ReadFile(reinterpret_cast<HANDLE>(file.handle), &value, sizeof(value), nullptr, nullptr);
    if (result_read == false)
        throw std::runtime_error("[ XDMA ] Invalid read data");

    return value;
}

// ----------------------------------------------------------------------------
// Запись регистра
// ----------------------------------------------------------------------------
void xdma::reg_write(xdma_file& file, size_t offset, uint32_t value)
{
    const std::lock_guard<std::mutex> lock(file.mutex);

    auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(file.handle), offset, 0, FILE_BEGIN);
    if (result_offset == INVALID_SET_FILE_POINTER)
        throw std::runtime_error("[ XDMA ] Invalid set offset");

    auto result_write = WriteFile(reinterpret_cast<HANDLE>(file.handle), &value, sizeof(value), nullptr, nullptr);
    if (result_write == false)
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

    DWORD read_bytes {};
    auto result_read = ReadFile(reinterpret_cast<HANDLE>(d_ptr->file_c2h.at(ch_num).handle), buf.data(), buf.size(), &read_bytes, nullptr);
    if (result_read == false)
        throw std::runtime_error("[ C2H ] Invalid read data");

    buf.resize(read_bytes);
    return buf;
}

// ----------------------------------------------------------------------------
// Чтение CFGROM
// ----------------------------------------------------------------------------
auto xdma::get_cfgrom() -> std::vector<uint8_t>
{
    std::vector<uint8_t> cfgrom(16384, 0);
    DWORD read_bytes {};

    const std::lock_guard<std::mutex> lock(d_ptr->file_user.mutex);
    auto result_offset = SetFilePointer(reinterpret_cast<HANDLE>(d_ptr->file_user.handle), 0, 0, FILE_BEGIN);
    auto result_read = ReadFile(reinterpret_cast<HANDLE>(d_ptr->file_user.handle), cfgrom.data(), cfgrom.size(), &read_bytes, nullptr);

    if ((result_offset == INVALID_SET_FILE_POINTER) || (result_read == false))
        throw std::runtime_error("[ XDMA ] Invalid read data from CFGROM");

    cfgrom.resize(read_bytes);
    return cfgrom;
}
