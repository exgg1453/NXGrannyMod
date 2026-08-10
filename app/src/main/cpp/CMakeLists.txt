cmake_minimum_required(VERSION 3.22.1)

project(nxgranny)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(nxgranny SHARED
        main.cpp
        il2cpp.cpp
        unity.cpp
        config.cpp
        mod.cpp)

find_package(shadowhook REQUIRED CONFIG)

target_link_libraries(nxgranny
        shadowhook::shadowhook
        log
        dl)

target_compile_options(nxgranny PRIVATE -Wall -fvisibility=hidden -ffunction-sections -fdata-sections)
target_link_options(nxgranny PRIVATE -Wl,--gc-sections)
