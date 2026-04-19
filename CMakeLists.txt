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

set(GENERATE_PY ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate.py)

set(GPERF_DIR ${CMAKE_CURRENT_BINARY_DIR}/gperf)
set(EVENT_CODES_MAP_GPERF ${GPERF_DIR}/event_codes_map.gperf)
set(EVENT_CODES_MAP_CPP ${GPERF_DIR}/event_codes_map.cpp)
set(EVENT_CODES_MAP_H ${GPERF_DIR}/event_codes_map.h)
set(GENERATE_PY_OUTPUT ${EVENT_CODES_MAP_GPERF} ${EVENT_CODES_MAP_CPP} ${EVENT_CODES_MAP_H})

set(SRC_FILES 
    src/main.cpp
    src/device.cpp
    src/device_grabber.cpp
    src/virtual_device.cpp
    src/event_pipeline.cpp
    src/lua_runtime.cpp
    ${EVENT_CODES_MAP_CPP}
)

add_custom_command(
    OUTPUT ${GENERATE_PY_OUTPUT}
    COMMAND python3 ${GENERATE_PY} ${EVENT_CODES_MAP_GPERF}
    DEPENDS ${GENERATE_PY}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

add_executable(keylua ${SRC_FILES})

target_include_directories(keylua PRIVATE src ${GPERF_DIR})
target_link_libraries(keylua PkgConfig::EVDEV PkgConfig::UDEV PkgConfig::LUA)
target_compile_options(keylua PRIVATE 
    -Wall -Wextra -Werror -pedantic-errors -Wsign-conversion
)
target_compile_options(keylua PRIVATE 
    -Wno-unused-variable
)


