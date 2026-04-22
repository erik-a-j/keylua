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


set(BUILD_GPERF_DIR ${CMAKE_CURRENT_BINARY_DIR}/gperf)
set(BUILD_KEYLUA_DIR ${CMAKE_CURRENT_BINARY_DIR}/types)

set(EVENT_CODES_MAP_NAME "event_codes_map")
set(ALIAS_EVENT_CODES_MAP_NAME "alias_event_codes_map")
set(KEYLUA_NAME "keylua")

set(EVENT_CODES_MAP_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/${EVENT_CODES_MAP_NAME}.cpp)

set(EVENT_CODES_MAP_GPERF_IN ${CMAKE_CURRENT_SOURCE_DIR}/gperf/${EVENT_CODES_MAP_NAME}.gperf.in)
set(ALIAS_EVENT_CODES_MAP_GPERF_IN ${CMAKE_CURRENT_SOURCE_DIR}/gperf/${ALIAS_EVENT_CODES_MAP_NAME}.gperf.in)
set(KEYLUA_LUA_IN ${CMAKE_CURRENT_SOURCE_DIR}/types/${KEYLUA_NAME}.lua.in)

set(EVENT_CODES_MAP_GPERF ${BUILD_GPERF_DIR}/${EVENT_CODES_MAP_NAME}.gperf)
set(ALIAS_EVENT_CODES_MAP_GPERF ${BUILD_GPERF_DIR}/${ALIAS_EVENT_CODES_MAP_NAME}.gperf)
set(GENERATED_EVENT_CODES_MAP_CPP ${BUILD_GPERF_DIR}/${EVENT_CODES_MAP_NAME}.cpp)
set(EVENT_CODES_MAP_H ${BUILD_GPERF_DIR}/${EVENT_CODES_MAP_NAME}.h)
set(KEYLUA_LUA ${BUILD_KEYLUA_DIR}/${KEYLUA_NAME}.lua)

set(GENERATE_PY_OUTPUT 
    ${EVENT_CODES_MAP_GPERF} 
    ${ALIAS_EVENT_CODES_MAP_GPERF} 
    ${GENERATED_EVENT_CODES_MAP_CPP} 
    ${EVENT_CODES_MAP_H}
    ${KEYLUA_LUA}
)
set(GENERATE_PY ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate.py)
set(GPERF_ITERATIONS 100)
set(GENERATE_PY_ARGS
    --gperf-iterations ${GPERF_ITERATIONS}
    --gperf-in ${EVENT_CODES_MAP_GPERF_IN}
    --gperf-out ${EVENT_CODES_MAP_GPERF}
    --gperf-alias-in ${ALIAS_EVENT_CODES_MAP_GPERF_IN}
    --gperf-alias-out ${ALIAS_EVENT_CODES_MAP_GPERF}
    --cpp-out ${GENERATED_EVENT_CODES_MAP_CPP}
    --h-out ${EVENT_CODES_MAP_H}
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
    ${GENERATED_EVENT_CODES_MAP_CPP}
    ${EVENT_CODES_MAP_CPP}
)

add_custom_command(
    OUTPUT ${GENERATE_PY_OUTPUT}
    COMMAND python3 ${GENERATE_PY} ${GENERATE_PY_ARGS}
    DEPENDS ${GENERATE_PY} ${EVENT_CODES_MAP_GPERF_IN} ${ALIAS_EVENT_CODES_MAP_GPERF_IN} ${KEYLUA_LUA_IN}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM
)

add_executable(keylua ${SRC_FILES})

target_include_directories(keylua PRIVATE src ${BUILD_GPERF_DIR})
target_link_libraries(keylua PkgConfig::EVDEV PkgConfig::UDEV PkgConfig::LUA)
target_compile_options(keylua PRIVATE 
    -Wall -Wextra -Werror -pedantic-errors -Wsign-conversion
)
target_compile_options(keylua PRIVATE 
    -Wno-unused-variable
)


