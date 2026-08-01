cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")

project(vw_spike CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CMake ships `import std` support only for MSVC (std.ixx) and Clang+libc++.
# Clang targeting the MSVC ABI uses MS STL, so its `std` module is built by us
# from the same std.ixx — see std/CMakeLists.txt.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_MODULE_STD ON)
else()
    set(CMAKE_CXX_MODULE_STD OFF)
endif()

enable_testing()

add_subdirectory(std)
add_subdirectory(core)
add_subdirectory(vulkan)
add_subdirectory(app)
add_subdirectory(tests)
