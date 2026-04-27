cmake_minimum_required(VERSION 3.22)
project(keylua LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
find_package(Python3 COMPONENTS Interpreter)

pkg_check_modules(EVDEV REQUIRED IMPORTED_TARGET libevdev)
pkg_check_modules(SYSTEMD REQUIRED IMPORTED_TARGET libsystemd)
pkg_check_modules(LUA REQUIRED IMPORTED_TARGET lua)

set(KEYLUA_LUA_IN ${CMAKE_CURRENT_SOURCE_DIR}/types/keylua.lua.in)
set(KEYLUA_LUA ${CMAKE_CURRENT_BINARY_DIR}/types/keylua.lua)

set(GENERATE_PY ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_lua.py)
set(GENERATE_PY_OUTPUT ${KEYLUA_LUA})

set(GENERATE_PY_ARGS
    --lua-in ${KEYLUA_LUA_IN}
    --lua-out ${KEYLUA_LUA}
)

set(SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/device_grabber.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/virtual_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/event_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lua_runtime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/monitor.cpp
)

add_custom_command(
    OUTPUT ${GENERATE_PY_OUTPUT}
    COMMAND Python3::Interpreter ${GENERATE_PY} ${GENERATE_PY_ARGS}
    DEPENDS ${GENERATE_PY} ${KEYLUA_LUA_IN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)
add_custom_target(generate_lua
    DEPENDS ${GENERATE_PY_OUTPUT}
    COMMENT "Intermediate keylua.lua target"
    VERBATIM
)

add_executable(keylua ${SRC_FILES})
add_dependencies(keylua generate_lua)

target_link_libraries(keylua PkgConfig::EVDEV PkgConfig::SYSTEMD PkgConfig::LUA)
target_compile_options(keylua PRIVATE 
    -Wall -Wextra -Werror -pedantic-errors -Wsign-conversion
)
target_compile_options(keylua PRIVATE 
    -Wno-unused-variable
)


