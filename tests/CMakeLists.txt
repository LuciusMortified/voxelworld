cmake_minimum_required(VERSION 4.0)
project(voxelworld_tests)

find_package(Catch2 3 CONFIG REQUIRED)

include(CTest)
include(Catch)

# Tests link the module libraries directly, never vwengine: nothing here needs
# a GPU, and a headless configuration must not drag Vulkan into the link.
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
    core/log.test.cpp
)

target_link_libraries(core_tests PRIVATE
    vw_core
    Catch2::Catch2WithMain
)

target_compile_features(core_tests PRIVATE cxx_std_23)
vw_use_std_module(core_tests)

catch_discover_tests(core_tests)

add_executable(ecs_tests
    ecs/entity_pool.test.cpp
    ecs/component_pool.test.cpp
    ecs/registry.test.cpp
    ecs/socket.test.cpp
    ecs/animation_layer.test.cpp
    ecs/dynamic_aabb_tree.test.cpp
)

target_link_libraries(ecs_tests PRIVATE
    vw_core
    vw_ecs
    vw_world
    Catch2::Catch2WithMain
)

target_compile_features(ecs_tests PRIVATE cxx_std_23)
vw_use_std_module(ecs_tests)

catch_discover_tests(ecs_tests)

add_executable(world_tests world/headless.test.cpp)

target_link_libraries(world_tests PRIVATE
    vw_core
    vw_ecs
    vw_world
    Catch2::Catch2WithMain
)

target_compile_features(world_tests PRIVATE cxx_std_23)
vw_use_std_module(world_tests)

catch_discover_tests(world_tests)

add_executable(view_bench ecs/view_bench.cpp)
target_link_libraries(view_bench PRIVATE vw_core vw_ecs)
target_compile_features(view_bench PRIVATE cxx_std_23)
vw_use_std_module(view_bench)
