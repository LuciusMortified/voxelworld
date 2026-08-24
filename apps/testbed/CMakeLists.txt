project(testbed)

find_package(imgui CONFIG REQUIRED)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/testbed.cppm
            src/options.cppm
            src/scene.cppm
            src/probes/clusters.cppm
            src/app.cppm
            src/scenes.cppm
            src/scenes/traverse.cppm
            src/scenes/dig.cppm
            src/scenes/light.cppm
            src/scenes/torches.cppm
            src/scenes/blobs.cppm
            src/scenes/crowd.cppm
    PRIVATE
        src/main.cpp
        src/scenes.cpp
        src/probes/clusters.cpp
        src/app/app.cpp
        src/app/world_setup.cpp
        src/app/daynight.cpp
        src/app/editing.cpp
        src/app/ui.cpp
        src/scenes/traverse.cpp
        src/scenes/dig.cpp
        src/scenes/light.cpp
        src/scenes/torches.cpp
        src/scenes/blobs.cpp
        src/scenes/crowd.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine imgui::imgui)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
vw_use_std_module(${PROJECT_NAME})
vw_set_warnings(${PROJECT_NAME})

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

include(shaders)
vw_setup_shaders(${PROJECT_NAME})
