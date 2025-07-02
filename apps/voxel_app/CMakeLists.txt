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
add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMENT "Copying shaders to ${PROJECT_NAME} build directory"
)
