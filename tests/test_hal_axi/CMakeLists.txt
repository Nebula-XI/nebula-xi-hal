cmake_minimum_required(VERSION 3.19)

project(nebula-xi-hal-axi-test)

set(PROJECT_COMPILE_OPTIONS)
set(PROJECT_COMPILE_DEFINES)
set(PROJECT_LINK_LIBRARIES nebula-xi-hal)
set(PROJECT_COMPILE_FEATURES cxx_std_17)

add_executable(${PROJECT_NAME} test_hal_axi.cpp)
target_sources(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCES})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIRS})

target_compile_features(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_FEATURES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_DEFINES})
target_compile_options(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_OPTIONS})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBRARIES})

if(UNIX)
    target_link_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Release>:-s>")
    set(CMAKE_SKIP_RPATH on)
    target_link_options(${PROJECT_NAME} PUBLIC "-Wl,-rpath=.")
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME} )

