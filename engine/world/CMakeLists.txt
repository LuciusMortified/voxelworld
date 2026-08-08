add_library(vw_world STATIC
    src/world.cpp
    src/animation_fsm_system.cpp
    src/animation_system.cpp
    src/character_controller_system.cpp
    src/hierarchy_system.cpp
    src/light_system.cpp
    src/model_system.cpp
    src/physics_system.cpp
    src/socket_system.cpp
    src/spatial_system.cpp
    src/transform_system.cpp
    src/world_grid_system.cpp
    src/chunk.cpp
    src/world_grid.cpp
)

target_include_directories(vw_world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(vw_world PUBLIC vw_core vw_ecs)
target_compile_features(vw_world PUBLIC cxx_std_23)
