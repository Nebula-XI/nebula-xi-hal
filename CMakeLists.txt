cmake_minimum_required(VERSION 3.19)
project(nebula-xi-hal)

set(PROJECT_COMPILE_FEATURES cxx_std_17)
set(PROJECT_COMPILE_DEFINES)

set(PROJECT_INCLUDE_DIRS
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/src
)
set(PROJECT_INCLUDES)

if(WIN32)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} ${PROJECT_SOURCE_DIR}/src/hal_windows.cpp)
    set(PROJECT_LINK_LIBRARIES setupapi)
else()
    set(PROJECT_SOURCES ${PROJECT_SOURCES} ${PROJECT_SOURCE_DIR}/src/hal_linux.cpp)
endif()
set(PROJECT_SOURCES ${PROJECT_SOURCES} ${PROJECT_SOURCE_DIR}/src/hal_common.cpp)

option(NEBULAXI_HAL_TEST_VERBOSE "Verbose output nebula-xi-hal test." OFF)

if(NEBULAXI_HAL_TEST_VERBOSE)
    set(PROJECT_COMPILE_DEFINES ${PROJECT_COMPILE_DEFINES} NEBULAXI_HAL_TEST_VERBOSE)
endif()


add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE on) # -fPIC

target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_INCLUDE_DIRS}>")

target_compile_features(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_FEATURES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_OPTIONS})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBRARIES})

option(NEBULAXI_HAL_BUILD_TEST "Build nebula-xi-hal test." OFF)

if(NEBULAXI_HAL_BUILD_TEST)
    enable_testing()
    add_subdirectory(tests)
endif()
