project(arena LANGUAGES CXX)

find_package(imgui CONFIG REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/arena.cppm
    PRIVATE
        src/main.cpp
        src/app/arena_app.cpp
        src/app/dummy_enemy.cpp
        src/app/player.cpp
        src/app/world_setup.cpp
        src/app/debug_hud.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine imgui::imgui)
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
