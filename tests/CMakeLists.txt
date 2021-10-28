cmake_minimum_required(VERSION 3.19)

project(nebula-xi-hal-tests)

set(PROJECT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/ctest/)

add_subdirectory(test_hal_axi)
