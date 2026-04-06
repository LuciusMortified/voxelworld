project(test_player_controller)

add_executable(${PROJECT_NAME}
    main.cpp
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

include(shaders)
vw_setup_shaders(${PROJECT_NAME})

include(assets)
vw_setup_assets(${PROJECT_NAME})
