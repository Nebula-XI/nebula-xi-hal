#include <iostream>

#ifdef __linux__
#define CTEST_SEGFAULT
#endif
#define CTEST_MAIN
#define CTEST_COLOR_OK
#include <ctest.h>

#include "nebulaxi/hal/hal_axi.h"

#ifndef NEBULAXI_HAL_TEST_VERBOSE
#define NEBULAXI_HAL_TEST_VERBOSE false
#endif

using namespace nebulaxi;

static constexpr bool g_verbose { NEBULAXI_HAL_TEST_VERBOSE };

CTEST_DATA(nebulaxi_hal) {

};

CTEST_SETUP(nebulaxi_hal)
{
}

CTEST_TEARDOWN(nebulaxi_hal)
{
}

CTEST2(nebulaxi_hal, axi)
{
    auto device_path_info_list = hal_device::get_path_info_list();
    if (device_path_info_list.empty()) {
        ASSERT_FAIL();
    }
    ASSERT_STR(device_path_info_list.at(0).first.c_str(), "/dev/xdma0");
}

int main(int argc, const char** argv)
{
    return ctest_main(argc, argv);
}
