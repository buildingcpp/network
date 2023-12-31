cmake_minimum_required(VERSION 3.16.3)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME network)
project(${PROJECT_NAME})

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(
        -g
        -O0
        -march=native
    )
else()
    add_compile_options(
        -O3
        -march=native
        -g
    )
endif()

# uncomment if you wish to build kqueue by default (requires) the following:
# sudo apt-get install libkqeuue-dev
# sudo apt-get install libevent-dev

# add_compile_definitions(USE_KQUEUE)

include("./scripts/fetch_dependencies.cmake")

fetch_dependency("https://github.com/buildingcpp/include.git;main")
fetch_dependency("https://github.com/buildingcpp/system.git;main")

message("Build Type = ${CMAKE_BUILD_TYPE}")

option(NETWORK_BUILD_DEMO "Build examples" ON)
option(NETWORK_BUILD_TEST "Build tests" ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME})
set(_${PROJECT_NAME}_dir ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "")

add_subdirectory(src)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
