project(arena LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/app/arena_app.h
    src/app/arena_app.inl.h
    src/app/dummy_enemy.h
    src/app/dummy_enemy.inl.h
    src/app/player.h
    src/app/player.inl.h
    src/app/world_setup.h
    src/app/world_setup.inl.h
    src/app/debug_hud.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

include(shaders)
vw_setup_shaders(${PROJECT_NAME})

include(assets)
vw_setup_assets(${PROJECT_NAME})
