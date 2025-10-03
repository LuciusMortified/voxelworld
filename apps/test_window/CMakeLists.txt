project(test_window)

# Тестовое приложение для проверки работы окна
add_executable(${PROJECT_NAME} 
    main.cpp
)

# Линковка с движком
target_link_libraries(${PROJECT_NAME} vwengine)

# Компиляционные флаги - C++23
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

# Копирование шейдеров не требуется для test_window