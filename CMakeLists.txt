cmake_minimum_required(VERSION 3.22)
project(keylua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(EVDEV REQUIRED IMPORTED_TARGET libevdev)
pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua)

set(SRC_FILES 
    src/main.cpp
    src/device.cpp
    src/device_grabber.cpp
    src/virtual_device.cpp
    src/event_pipeline.cpp
    src/lua_runtime.cpp
)

add_executable(keylua ${SRC_FILES})

target_link_libraries(keylua PkgConfig::EVDEV PkgConfig::UDEV PkgConfig::LUA)
target_compile_options(keylua PRIVATE 
    -Wall -Wextra -Werror -pedantic-errors -Wconversion -Wsign-conversion
)
target_compile_options(keylua PRIVATE 
    -Wno-unused-variable
)


