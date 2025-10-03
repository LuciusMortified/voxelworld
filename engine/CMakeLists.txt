cmake_minimum_required(VERSION 3.16)

project(vwengine)

# Engine library
set(ENGINE_HEADERS
    "include/vw/types.h"
    "include/vw/math.h"
    "include/vw/vec3.h"
    "include/vw/common.h"
    "include/vw/mat4.h"
    "include/vw/gfx/color.h"
    "include/vw/gfx/events.h"
    "include/vw/gfx/engine.h"
    "include/vw/gfx/voxel.h"
    "include/vw/gfx/world.h"
    "include/vw/gfx/transform.h"
    "include/vw/gfx/model.h"
    "include/vw/gfx/window.h"
    "include/vw/gfx/input.h"
    "include/vw/gfx/vulkan_context.h"
    "include/vw/gfx/camera.h"
    "include/vw/gfx/camera_controller.h"
    "include/vw/gfx/app.h"
    "include/vw/gfx/buffer.h"
    "include/vw/gfx/mesh.h"
    "include/vw/gfx/shader.h"
    "include/vw/gfx/renderer.h"
    "include/vw/gfx/debug_tool.h"
    "include/vw/gfx/debug_primitive.h"
)

set(ENGINE_SOURCES
    "src/math.cpp"
    "src/gfx/engine.cpp"
    "src/gfx/world.cpp"
    "src/gfx/transform.cpp"
    "src/gfx/model.cpp"
    "src/gfx/window.cpp"
    "src/gfx/buffer.cpp"
    "src/gfx/mesh.cpp"
    "src/gfx/vulkan_context.cpp"
    "src/gfx/camera.cpp"
    "src/gfx/camera_controller.cpp"
    "src/gfx/renderer.cpp"
    "src/gfx/shader.cpp"
    "src/gfx/debug_tool.cpp"
    "src/gfx/debug_primitive.cpp"
        include/vw/vec4.h
)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Create engine library
add_library(${PROJECT_NAME} STATIC ${ENGINE_HEADERS} ${ENGINE_SOURCES})

# Set include directories
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARIES} glfw imgui::imgui)

# Compile features - C++23
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# Compile definitions
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
endif()
