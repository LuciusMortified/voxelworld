cmake_minimum_required(VERSION 3.20)
project(voxelworld_apps)

# Apps directory - отдельные приложения на движке

# Подключение отдельных приложений
add_subdirectory(voxel_app)
add_subdirectory(test_window)

# Здесь можно добавлять другие приложения
# add_subdirectory(another_app)