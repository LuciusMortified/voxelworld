cmake_minimum_required(VERSION 3.16)

project(vwengine)

# Engine library
set(ENGINE_HEADERS
    "include/vw/core/types.h"
    "include/vw/core/math.h"
    "include/vw/core/vec3.h"
    "include/vw/core.h"
    "include/vw/core/mat4.h"
    "include/vw/core/color.h"
    "include/vw/gfx/window/event.h"
    "include/vw/gfx/engine/engine.h"
    "include/vw/core/voxel.h"
    "include/vw/gfx/world/world.h"
    "include/vw/gfx/world/transform.h"
    "include/vw/gfx/resource/model.h"
    "include/vw/gfx/window/window.h"
    "include/vw/gfx/window/input.h"
    "include/vw/gfx/render/vulkan_context.h"
    "include/vw/gfx/camera/camera.h"
    "include/vw/gfx/camera/camera_controller.h"
    "include/vw/gfx/engine/app.h"
    "include/vw/gfx/resource/buffer.h"
    "include/vw/gfx/resource/mesh.h"
    "include/vw/gfx/resource/shader.h"
    "include/vw/gfx/render/renderer.h"
    "include/vw/gfx/debug/debug_window.h"
    "include/vw/gfx/debug/debug_primitive.h"
        include/vw/core/vec2.h
        include/vw/gfx.h
)

set(ENGINE_SOURCES
    "src/core/math.cpp"
    "src/gfx/engine/engine.cpp"
    "src/gfx/world/world.cpp"
    "src/gfx/world/transform.cpp"
    "src/gfx/resource/model.cpp"
    "src/gfx/window/window.cpp"
    "src/gfx/resource/buffer.cpp"
    "src/gfx/resource/mesh.cpp"
    "src/gfx/render/vulkan_context.cpp"
    "src/gfx/camera/camera.cpp"
    "src/gfx/camera/camera_controller.cpp"
    "src/gfx/render/renderer.cpp"
    "src/gfx/resource/shader.cpp"
    "src/gfx/debug/debug_window.cpp"
    "src/gfx/debug/debug_primitive.cpp"
        include/vw/core/vec4.h
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
