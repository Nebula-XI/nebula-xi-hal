#include "nebulaxi/hal/hal_axi.h"
#include "nebulaxi/hal/hal_pcie.h"

using namespace nebulaxi;

auto hal_pcie::read(reg_offset offset) const -> reg_value
{
    return hal_device::read<io_name>(d_ptr->file_control, offset);
}

void hal_pcie::write(reg_offset offset, reg_value value) const
{
    hal_device::write<io_name>(d_ptr->file_control, offset, value);
}

auto hal_pcie::get_location() const noexcept -> std::string
{
    auto value = read(0);
    std::string location {};
    location += std::to_string(d_ptr->dev_info.bus);
    location += std::to_string(d_ptr->dev_info.dev);
    location += std::to_string(d_ptr->dev_info.func);
    return location;
}

auto hal_axi::read(reg_offset offset) const -> reg_value
{
    return hal_device::read<io_name>(d_ptr->file_user, offset);
}

void hal_axi::write(reg_offset offset, reg_value value) const
{
    hal_device::write<io_name>(d_ptr->file_user, offset, value);
};
