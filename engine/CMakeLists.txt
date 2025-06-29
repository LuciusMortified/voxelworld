cmake_minimum_required(VERSION 3.16)

project(voxelengine)

# Engine library
set(ENGINE_HEADERS
    "include/voxel/engine.h"
    "include/voxel/types.h"
    "include/voxel/voxel.h"
    "include/voxel/world.h"
    "include/voxel/transform.h"
    "include/voxel/model.h"
    "include/voxel/window.h"
    "include/voxel/input.h"
    "include/voxel/vulkan_context.h"
    "include/voxel/camera.h"
    "include/voxel/camera_controller.h"
    "include/voxel/game_logic.h"
    "include/voxel/buffer.h"
    "include/voxel/mesh.h"
    "include/voxel/shader.h"
    "include/voxel/renderer.h"
    "include/voxel/math_utils.h"
    "include/voxel/events.h"
)

set(ENGINE_SOURCES
    "src/engine.cpp"
    "src/world.cpp"
    "src/transform.cpp"
    "src/model.cpp"
    "src/window.cpp"
    "src/buffer.cpp"
    "src/mesh.cpp"
    "src/vulkan_context.cpp"
    "src/camera.cpp"
    "src/camera_controller.cpp"
    "src/game_logic.cpp"
    "src/math_utils.cpp"
    "src/renderer.cpp"
    "src/shader.cpp"
    "src/events.cpp"
)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Create engine library
add_library(${PROJECT_NAME} STATIC ${ENGINE_HEADERS} ${ENGINE_SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} glfw glm::glm)

# Compile features - C++20
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

# Compile definitions
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
endif()

# Copy shaders to build directory
add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Copying shaders to build directory"
)