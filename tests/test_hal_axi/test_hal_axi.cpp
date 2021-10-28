#include <iomanip>
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

CTEST(nebulaxi, cfgrom)
{
    auto get_cfgrom = [] {
        auto hal_axi = make_hal_axi("/dev/xdma0");
        std::vector<uint8_t> cfgrom {};
        bool eof {};
        while (!eof) {
            auto value = hal_axi->read(cfgrom.size());
            if (value == 0)
                break;
            for (auto it : { 0, 8, 16, 24 }) {
                auto byte = (value & (0xFF << it)) >> it;
                if (byte != 0) {
                    cfgrom.push_back(byte);
                } else {
                    eof = true;
                }
            }
        }
        return std::string { cfgrom.begin(), cfgrom.end() };
    };
    std::cout << get_cfgrom() << '\n';
}

int main(int argc, const char** argv)
{
    return ctest_main(argc, argv);
}
