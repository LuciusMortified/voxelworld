add_library(vw_world STATIC)

target_sources(vw_world
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/world.cppm
            src/model.cppm
            src/anim.cppm
            src/serial.cppm
            src/index.cppm
            src/components.cppm
            src/terrain.cppm
            src/grid.cppm
            src/systems.cppm
    PRIVATE
        src/model.cpp
        src/anim.cpp
        src/serial.cpp
        src/index.cpp
        src/terrain.cpp
        src/scene_serial.cpp
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

target_link_libraries(vw_world PUBLIC vw_core vw_ecs)
target_compile_features(vw_world PUBLIC cxx_std_23)
vw_use_std_module(vw_world)
