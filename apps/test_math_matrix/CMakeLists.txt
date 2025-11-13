project(test_math_matrix)

# Основное приложение воксельного движка
add_executable(${PROJECT_NAME}
        main.cpp
)

# Линковка с движком
target_link_libraries(${PROJECT_NAME} vwengine)

# Компиляционные флаги - C++23
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Копирование шейдеров из движка в папку приложения
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
        COMMENT "Copying shaders to ${PROJECT_NAME} build directory"
)
