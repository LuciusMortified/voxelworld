project(test_world_grid)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

include(shaders)
vw_setup_shaders(${PROJECT_NAME})
