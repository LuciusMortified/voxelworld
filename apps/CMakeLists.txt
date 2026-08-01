cmake_minimum_required(VERSION 4.0)
project(voxelworld_apps)

# Apps directory - отдельные приложения на движке

add_subdirectory(test_simple_model)
add_subdirectory(test_window)
add_subdirectory(test_animation)
add_subdirectory(test_lighting)
add_subdirectory(test_world_grid)
add_subdirectory(arena)

add_subdirectory(sculptor)