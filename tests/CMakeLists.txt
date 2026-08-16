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

add_executable(world_tests
    world/headless.test.cpp
    world/column_staging.test.cpp
    world/model_fill.test.cpp
    world/surface.test.cpp
    world/caves.test.cpp
    world/chunk_links.test.cpp
    world/chunk_walk.test.cpp
    world/cave_culling.test.cpp
)

target_link_libraries(world_tests PRIVATE
    vw_core
    vw_ecs
    vw_world
    Catch2::Catch2WithMain
)

target_compile_features(world_tests PRIVATE cxx_std_23)
vw_use_std_module(world_tests)

catch_discover_tests(world_tests)

# The mesher is CPU-only but lives in vw.gfx, so its tests link the graphics
# library and are skipped in a headless configuration. They never open a device.
if(VW_BUILD_GFX)
    add_executable(gfx_tests gfx/mesher.test.cpp)

    target_link_libraries(gfx_tests PRIVATE
        vw_core
        vw_ecs
        vw_world
        vw_gfx
        Catch2::Catch2WithMain
    )

    target_compile_features(gfx_tests PRIVATE cxx_std_23)
    vw_use_std_module(gfx_tests)

    catch_discover_tests(gfx_tests)
endif()

add_executable(view_bench ecs/view_bench.cpp)
target_link_libraries(view_bench PRIVATE vw_core vw_ecs)
target_compile_features(view_bench PRIVATE cxx_std_23)
vw_use_std_module(view_bench)
