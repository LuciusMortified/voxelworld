project(test_lighting)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
vw_use_std_module(${PROJECT_NAME})
vw_set_warnings(${PROJECT_NAME})

include(shaders)
vw_setup_shaders(${PROJECT_NAME})
