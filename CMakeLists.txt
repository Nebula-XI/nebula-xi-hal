# -----------------------------------------------------------------------------
# XDMA HAL Library
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.19)
project(xdma)

set(PROJECT_COMPILE_FEATURES cxx_std_17)
# set(PROJECT_COMPILE_OPTIONS -Wall)
# set(PROJECT_COMPILE_OPTIONS ${PROJECT_COMPILE_OPTIONS} -Wno-c++98-compat -Wno-c++98-compat-pedantic)
# set(PROJECT_COMPILE_OPTIONS ${PROJECT_COMPILE_OPTIONS} -Wno-global-constructors -Wno-exit-time-destructors -Wno-format-nonliteral -Wno-return-std-move)
set(PROJECT_COMPILE_DEFINES)

set(PROJECT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_INCLUDES)

if(WIN32)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} ${PROJECT_SOURCE_DIR}/src/windows.cpp)
    set(PROJECT_LINK_LIBRARIES setupapi)
else()
    set(PROJECT_SOURCES ${PROJECT_SOURCES} ${PROJECT_SOURCE_DIR}/src/linux.cpp)
endif()

# -----------------------------------------------------------------------------
# Make Static Library
# -----------------------------------------------------------------------------
add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE on) # -fPIC

target_include_directories(${PROJECT_NAME} PUBLIC "$<BUILD_INTERFACE:${PROJECT_INCLUDE_DIRS}>")

target_compile_features(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_FEATURES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_OPTIONS})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBRARIES})
