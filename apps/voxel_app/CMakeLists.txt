project(voxel_app)

# Основное приложение воксельного движка
add_executable(${PROJECT_NAME} 
    main.cpp
)

# Линковка с движком
target_link_libraries(${PROJECT_NAME} voxelengine)

# Компиляционные флаги - C++20
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Копирование шейдеров не требуется для voxel_app
