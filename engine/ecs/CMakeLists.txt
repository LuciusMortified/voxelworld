add_library(vw_ecs STATIC)

target_sources(vw_ecs
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/ecs.cppm
            src/entity.cppm
            src/pool.cppm
            src/registry.cppm
    PRIVATE
        src/entity.cpp
        src/pool.cpp
        src/registry.cpp
)

target_link_libraries(vw_ecs PUBLIC vw_core)
target_compile_features(vw_ecs PUBLIC cxx_std_23)
vw_use_std_module(vw_ecs)
vw_set_warnings(vw_ecs)
