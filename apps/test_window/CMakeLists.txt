project(test_window)

# Тестовое приложение для проверки работы окна
add_executable(${PROJECT_NAME} 
    test_window.cpp
)

# Линковка с движком
target_link_libraries(${PROJECT_NAME} voxelengine)

# Компиляционные флаги - C++20
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Копирование шейдеров не требуется для test_window