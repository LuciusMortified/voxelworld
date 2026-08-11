cmake_minimum_required(VERSION 4.0)

# Gate for `import std`; the value is tied to the CMake version and changes with it.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

project(voxelworld LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if(NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(FATAL_ERROR
        "Generator '${CMAKE_GENERATOR}' cannot build C++ modules with `import std`; "
        "configure with -G Ninja")
endif()

include(vw_std_module)
include(vw_vulkan_module)

option(VW_BUILD_GFX "Build the Vulkan renderer and everything that needs a GPU" ON)
option(VW_BUILD_APPS "Build applications" ON)
option(VW_BUILD_TESTS "Build unit tests" ON)

if(VW_BUILD_APPS AND NOT VW_BUILD_GFX)
    message(FATAL_ERROR "VW_BUILD_APPS needs VW_BUILD_GFX: every app opens a window")
endif()

enable_testing()

add_subdirectory(engine)

if(VW_BUILD_APPS)
    add_subdirectory(apps)
endif()

if(VW_BUILD_TESTS)
    add_subdirectory(tests)
endif()
