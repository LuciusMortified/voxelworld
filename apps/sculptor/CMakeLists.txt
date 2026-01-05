project(sculptor VERSION 0.0.1 LANGUAGES CXX)

add_executable(${PROJECT_NAME}
        src/main.cpp
        src/app/app.h
        src/app/app.inl.h
)

target_link_libraries(${PROJECT_NAME} vwengine)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app/sculptor_version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/include/sculptor_version.h
        @ONLY
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/include
)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMENT "Copying shaders to ${PROJECT_NAME} build directory"
)
