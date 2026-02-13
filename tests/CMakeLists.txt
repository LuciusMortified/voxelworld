cmake_minimum_required(VERSION 3.16)
project(voxelworld_tests)

find_package(Catch2 3 CONFIG REQUIRED)

include(CTest)
include(Catch)

add_executable(core_tests
    core/voxel.test.cpp
    core/color.test.cpp
    core/vec2.test.cpp
    core/vec3.test.cpp
    core/vec4.test.cpp
    core/quat.test.cpp
    core/mat4.test.cpp
    core/math.test.cpp
    core/transform.test.cpp
)

target_link_libraries(core_tests PRIVATE
    vwengine
    Catch2::Catch2WithMain
)

target_compile_features(core_tests PRIVATE cxx_std_23)

catch_discover_tests(core_tests)
