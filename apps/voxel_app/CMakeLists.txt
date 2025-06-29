project(voxel_app)

# Основное приложение воксельного движка
add_executable(${PROJECT_NAME} 
    main.cpp
)

# Линковка с движком
target_link_libraries(${PROJECT_NAME} voxelengine)

# Компиляционные флаги - C++20
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Копирование шейдеров из движка в папку приложения
add_custom_target(copy_shaders_${PROJECT_NAME} ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/engine/shaders
        ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMENT "Copying shaders to ${PROJECT_NAME} build directory"
)

# Зависимость от копирования шейдеров
add_dependencies(${PROJECT_NAME} copy_shaders_${PROJECT_NAME})
