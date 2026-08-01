cmake_minimum_required(VERSION 4.0)
project(voxelworld)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(VW_BUILD_APPS "Build applications" ON)
option(VW_BUILD_TESTS "Build unit tests" ON)

enable_testing()

add_subdirectory(engine)

if(VW_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(VW_BUILD_TESTS)
    add_subdirectory(tests)
endif()
