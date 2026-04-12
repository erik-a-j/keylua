cmake_minimum_required(VERSION 3.22)
project(keylua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(EVDEV REQUIRED IMPORTED_TARGET libevdev)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)

set(SRC_FILES 
    src/main.cpp
    src/device.cpp
)

add_executable(keylua ${SRC_FILES})

target_link_libraries(keylua PkgConfig::EVDEV PkgConfig::UDEV)
target_compile_options(keylua PRIVATE 
    -Wall -Wextra -Werror -pedantic-errors -Wconversion -Wsign-conversion
)
target_compile_options(keylua PRIVATE 
    -Wno-unused-variable
)


