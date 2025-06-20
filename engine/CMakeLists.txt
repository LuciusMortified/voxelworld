project(voxelengine)

# Engine library
set(ENGINE_HEADERS
    "include/engine.h"
    "include/types.h"
    "include/voxel.h"
    "include/world.h"
    "include/model.h"
    "include/window.h"
    "include/input.h"
    "include/vulkan_context.h"
    "include/camera.h"
    "include/buffer.h"
    "include/mesh.h"
    "include/shader.h"
    "include/renderer.h"
)

set(ENGINE_SOURCES
    "src/engine.cpp"
    "src/world.cpp"
    "src/model.cpp"
    "src/window.cpp"
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