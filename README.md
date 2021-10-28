## Hardware Abstraction Layer (HAL)

[![CMake](https://github.com/Nebula-XI/nebula-xi-hal/actions/workflows/cmake.yml/badge.svg)](https://github.com/Nebula-XI/nebula-xi-hal/actions/workflows/cmake.yml)

## Requirements

CMake 3.21, Ninja 1.10.2, GCC 10.3 (Linux Native Build) or MinGW Cross-Compiler 10.3 (Build for Windows on Linux)

## Configuring

### Linux Native x86_64

`cmake --preset linux-x64-release`

`cmake --preset linux-x64-debug`

### Windows on Linux x86_64

`cmake --preset windows-x64-release`

`cmake --preset windows-x64-debug`

## Building

### Linux Native x86_64


`cmake --build --preset linux-x64-release`

`cmake --build --preset linux-x64-release-rebuild`

`cmake --build --preset linux-x64-release-verbose`

`cmake --build --preset linux-x64-debug`

`cmake --build --preset linux-x64-debug-rebuild`

`cmake --build --preset linux-x64-debug-verbose`

### Windows on Linux x86_64


`cmake --build --preset windows-x64-release`

`cmake --build --preset windows-x64-release-rebuild`

`cmake --build --preset windows-x64-release-verbose`

`cmake --build --preset windows-x64-debug`

`cmake --build --preset windows-x64-debug-rebuild`

`cmake --build --preset windows-x64-debug-verbose`

## Testing

### Linux Native x86_64


`ctest --preset linux-x64-release`

`ctest --preset linux-x64-debug`

### Windows on Linux x86_64


`ctest --preset windows-x64-release`

`ctest --preset windows-x64-debug`

## All

### Linux

`./make.sh config build test`

## Examples

### Device Path and Info Example

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
### Result of Device Path and Info Example

```bash
    ------------------------------------
    device path: /dev/xdma0
    vendor: 0x4953
    bus: 0x04
    dev: 0x00

```
