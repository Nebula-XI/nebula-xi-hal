#include <iomanip>
#include <iostream>

#ifdef __linux__
#define CTEST_SEGFAULT
#endif
#define CTEST_MAIN
#define CTEST_COLOR_OK
#include <ctest.h>

#include "nebulaxi/hal/hal_xdma.h"

#ifndef NEBULAXI_HAL_TEST_VERBOSE
#define NEBULAXI_HAL_TEST_VERBOSE false
#endif

using namespace nebulaxi;

static constexpr bool g_verbose { NEBULAXI_HAL_TEST_VERBOSE };

CTEST(nebulaxi_hal, xdma)
{

}

int main(int argc, const char** argv)
{
    return ctest_main(argc, argv);
}
