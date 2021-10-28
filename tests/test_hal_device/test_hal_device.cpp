#include <iomanip>
#include <iostream>

#ifdef __linux__
#define CTEST_SEGFAULT
#endif
#define CTEST_MAIN
#define CTEST_COLOR_OK
#include <ctest.h>

#include "nebulaxi/hal/hal_device.h"

#ifndef NEBULAXI_HAL_TEST_VERBOSE
#define NEBULAXI_HAL_TEST_VERBOSE false
#endif

using namespace nebulaxi;

static constexpr bool g_verbose { NEBULAXI_HAL_TEST_VERBOSE };

CTEST(nebulaxi_hal, device)
{
    auto device_path_info_list = hal_device::get_path_info_list();
    if (device_path_info_list.empty()) {
        ASSERT_FAIL();
    }
    auto device_path_c_str = device_path_info_list.at(0).first.c_str();
    auto device_info = device_path_info_list.at(0).second;
    ASSERT_STR(device_path_c_str, "/dev/xdma0");
    ASSERT_EQUAL(device_info.vendor, 0x4953);
    if (g_verbose) {
        std::cout << '\n' + std::string(36, '-') + '\n';
        std::cout << "device path: " << device_path_c_str << '\n';
        std::cout << "vendor id: 0x" << std::hex << std::setfill('0') << device_info.vendor << '\n';
        std::cout << "bus: 0x" << std::setw(2) << device_info.bus << '\n';
        std::cout << "dev: 0x" << std::setw(2) << device_info.dev << '\n';
    }
}

int main(int argc, const char** argv)
{
    return ctest_main(argc, argv);
}
