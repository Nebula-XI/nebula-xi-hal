#include "nebulaxi/hal/hal_axi.h"
#include "nebulaxi/hal/hal_pcie.h"
#include "nebulaxi/hal/hal_xdma.h"

using namespace nebulaxi;

void hal_pcie::open(const std::string& path, const device_info& dev_info)
{
    using namespace std::string_literals;
    auto control_name = path + "\\control";
    auto handle = CreateFileA(control_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + control_name + "\"");
    }
    d_ptr->file_control.handle = static_cast<int>(reinterpret_cast<size_t>(handle));
    d_ptr->dev_info = dev_info;
    d_ptr->dev_path = path;
}
void hal_pcie::close() const noexcept
{
    if (d_ptr) {
        CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_control.handle));
    }
}

void hal_axi::open(const std::string& path)
{
    using namespace std::string_literals;
    auto user_name = path + "\\user";
    auto handle = CreateFileA(user_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("[ "s + std::string { io_name } + " ] Can't open file: \"" + user_name + "\"");
    }
    d_ptr->file_user.handle = static_cast<int>(reinterpret_cast<size_t>(handle));
}
void hal_axi::close() const noexcept
{
    if (d_ptr) {
        CloseHandle(reinterpret_cast<HANDLE>(d_ptr->file_user.handle));
    }
}

DEFINE_GUID(GUID_DEVINTERFACE_XDMA, 0x74c7e4a9, 0x6d5d, 0x4a70, 0xbc, 0x0d, 0x20, 0x69, 0x1d, 0xff, 0x9e, 0x9d);

device_path_list get_device_paths()
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

hal_xdma::hal_xdma(const std::string& path, const device_info& dev_info)
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

hal_xdma::~hal_xdma() noexcept
{
    for (auto num : { 0, 1, 2, 3 }) {
        CloseHandle(reinterpret_cast<HANDLE>(m_dma->file_c2h.at(num).handle));
        CloseHandle(reinterpret_cast<HANDLE>(m_dma->file_h2c.at(num).handle));
    }
}

auto hal_xdma::read(dma_channel channel, size_t size) const -> dma_buffer
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

auto hal_xdma::write(dma_channel num, const dma_buffer& buffer) const -> void
{
    // TODO:
}
