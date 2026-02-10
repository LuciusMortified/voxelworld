project(test_animation)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE vwengine)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders to ${PROJECT_NAME} build directory"
)
