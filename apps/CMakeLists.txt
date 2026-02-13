cmake_minimum_required(VERSION 3.20)
project(voxelworld_apps)

# Apps directory - отдельные приложения на движке

add_subdirectory(test_simple_model)
add_subdirectory(test_window)
add_subdirectory(test_animation)

add_subdirectory(sculptor)