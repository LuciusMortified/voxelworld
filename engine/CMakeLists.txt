cmake_minimum_required(VERSION 3.16)

project(vwengine)

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
        "include/vw/gfx/world/world.inl.h"
        "include/vw/core/transform.h"
        "include/vw/gfx/window/window.h"
        "include/vw/gfx/window/input.h"
        "include/vw/gfx/render/vulkan_context.h"
        "include/vw/gfx/camera/camera.h"
        "include/vw/gfx/camera/fps_camera_controller.h"
        "include/vw/gfx/engine/app.h"
        "include/vw/gfx/resource/buffer.h"
        "include/vw/gfx/resource/mesh.h"
        "include/vw/gfx/resource/shader.h"
        "include/vw/gfx/render/renderer.h"
        "include/vw/gfx/debug/debug_window.h"
        "include/vw/gfx/debug/debug_primitive.h"
        "include/vw/core/vec2.h"
        "include/vw/gfx.h"
        "include/vw/gfx/world/entity.h"
        "include/vw/gfx/world/entity_pool.h"
        "include/vw/gfx/world/component_pool.h"
        "include/vw/gfx/world/registry.h"
        "include/vw/core/transform.inl.h"
        "include/vw/core/color.inl.h"
        "include/vw/gfx/world/components/hierarchy_component.h"
        "include/vw/gfx/world/components/hierarchy_component.inl.h"
        "include/vw/gfx/world/components/model_component.h"
        "include/vw/gfx/world/components/transform_component.h"
        "include/vw/gfx/world/components/transform_component.inl.h"
        "include/vw/gfx/world/systems/hierarchy_system.h"
        "include/vw/gfx/world/systems/hierarchy_system.inl.h"
        "include/vw/gfx/world/systems/transform_system.h"
        "include/vw/gfx/world/systems/transform_system.inl.h"
        "include/vw/gfx/world/systems/model_system.h"
        "include/vw/gfx/world/systems/model_system.inl.h"
        "include/vw/gfx/world/world.inl.h"
        "include/vw/gfx/model/model.h"
        "include/vw/gfx/model/model.inl.h"
        "include/vw/gfx/model/model_identity.h"
        "include/vw/gfx/model/model_identity_pool.h"
        "include/vw/gfx/model/model_registry.h"
        "include/vw/gfx/model/model_registry.inl.h"
        "include/vw/gfx/resource/mesh_pool.h"
        "include/vw/gfx/resource/mesh_pool.inl.h"
        "include/vw/gfx/resource/combined_buffer.h"
        "include/vw/gfx/resource/combined_buffer_pool.h"
        "include/vw/gfx/resource/combined_buffer_pool.inl.h"
        "include/vw/gfx/debug/debug_window.inl.h"
        "include/vw/gfx/engine/engine.inl.h"
        "include/vw/gfx/world/world_components.h"
        "include/vw/gfx/resource/combined_buffer.inl.h"
        "include/vw/core/math.inl.h"
        "include/vw/gfx/camera/camera.inl.h"
        "include/vw/gfx/camera/fps_camera_controller.inl.h"
        "include/vw/gfx/render/vulkan_context.inl.h"
        "include/vw/gfx/window/window.inl.h"
        "include/vw/gfx/resource/buffer.inl.h"
        "include/vw/gfx/resource/mesh.inl.h"
        "include/vw/gfx/resource/shader.inl.h"
        "include/vw/log.h"
        "include/vw/log/logger.h"
        "include/vw/log/logger.inl.h"
)

set(ENGINE_SOURCES)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Create engine library
add_library(${PROJECT_NAME} INTERFACE
        include/vw/gfx/world/components/model_component.inl.h
        include/vw/gfx/world/entity_archetype.h
        include/vw/gfx/world/entity_archetype.inl.h
        include/vw/gfx/world/entity_builder.h
        include/vw/gfx/world/entity_guard.h
        include/vw/gfx/world/entity_builder.inl.h
        include/vw/gfx/world/entity_guard.inl.h
        include/vw/gfx/world/serializers/vox_serializer.h
        include/vw/gfx/world/serializers/vox_serializer.inl.h
        include/vw/gfx/world/serializers/vox_deserializer.h
        include/vw/gfx/world/serializers/vox_deserializer.inl.h)

# Set include directories
target_include_directories(${PROJECT_NAME} INTERFACE include)
target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME} INTERFACE
        ${Vulkan_LIBRARIES}
        glfw
        imgui::imgui
        spdlog::spdlog_header_only
)

# Compile features - C++23
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

