# Hardware Abstraction Layer (HAL)

[![CMake](https://github.com/Nebula-XI/nebula-xi-hal/actions/workflows/cmake.yml/badge.svg)](https://github.com/Nebula-XI/nebula-xi-hal/actions/workflows/cmake.yml)

## Requirements

CMake 3.21, Ninja 1.10.2, GCC 10.3

## Configuring

### Linux x86_64

`cmake --preset linux-x64-release`

`cmake --preset linux-x64-debug`

## Building

### Linux x86_64


`cmake --build --preset linux-x64-release`

`cmake --build --preset linux-x64-release-rebuild`

`cmake --build --preset linux-x64-release-verbose`

`cmake --build --preset linux-x64-debug`

`cmake --build --preset linux-x64-debug-rebuild`

`cmake --build --preset linux-x64-debug-verbose`

## Testing

### Linux x86_64


`ctest --preset linux-x64-release`

`ctest --preset linux-x64-debug`

## All

### Linux

`./make.sh config build test`

## Examples

### Get Device Path and Info

```c
    auto device_path_info_list = hal_device::get_path_info_list();
    for (auto& device_path_info : device_path_info_list) {
        auto device_path_c_str = device_path_info_list.at(0).first.c_str();
        auto device_info = device_path_info_list.at(0).second;
        std::cout << '\n' + std::string(36, '-') + '\n';
        std::cout << "device path: " << device_path_c_str << '\n';
        std::cout << std::hex << std::setfill('0');
        std::cout << "vendor: 0x" << device_info.vendor << '\n';
        std::cout << "bus: 0x" << std::setw(2) << device_info.bus << '\n';
        std::cout << "dev: 0x" << std::setw(2) << device_info.dev << '\n';
    }

```
### Result of Get Device Path and Info

```bash
    ------------------------------------
    device path: /dev/xdma0
    vendor: 0x4953
    bus: 0x04
    dev: 0x00

```

### Read JSON Configuration

```c

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

```

### Result of Read JSON Configuration

```bash

{"id":"Nebula-XI","name":"FMC126P","version":"3.1","units":[{"name":"C2H Switch","driver":"AXI4-Stream Switch","offset":"0x00010000"},{"name":"H2C Switch","driver":"AXI4-Stream Switch","offset":"0x00020000"},{"name":"PSD Generator","driver":"AXI Traffic Generator","offset":"0x00030000"},{"name":"I2C Root","driver":"AXI IIC","offset":"0x00040000","units":[{"name":"TCA9548A","driver":"TCA9548A","label":"D23","addr":"0x70","freq":"400000","channels":"8","units":[{"name":"FMC","channel":"0"},{"name":"POWER","channel":"1","units":[{"name":"INA219","driver":"INA219","label":"D33","addr":"0x40","freq":"400000","additional":{}},{"name":"LTC2991","driver":"LTC2991","label":"D31","addr":"0x90","freq":"400000","additional":{}}]},{"name":"EXAR","channel":"5","units":[{"name":"XR77128","driver":"XR77128","label":"D32","addr":"0x28","freq":"400000"}]},{"name":"DDR4","channel":"6","units":[{"name":"DDR4 SODIMM","driver":"DDR4 SODIMM","label":"XS1.1","addr":"0x50","freq":"400000"}]},{"name":"SWITCH CLOCK","channel":"7","units":[{"name":"ADN4600","driver":"ADN4600","label":"D13","addr":"0x48","freq":"400000"}]}]}]}]}

```
