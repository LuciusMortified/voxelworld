find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

vw_add_vulkan_module()

add_library(vw_gfx STATIC)

target_sources(vw_gfx
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/gfx.cppm
            src/vk.cppm
            src/camera.cppm
            src/meshing.cppm
            src/mesh_pool.cppm
            src/gpu_buffers.cppm
            src/geometry.cppm
            src/resource.cppm
            src/render.cppm
            src/renderer.cppm
            src/engine.cppm
    PRIVATE
        src/camera.cpp
        src/camera_controllers.cpp
        src/shader.cpp
        src/buffer.cpp
        src/staging_buffer.cpp
        src/deletion_queue.cpp
        src/mesh.cpp
        src/mesh_pool.cpp
        src/palette_buffer.cpp
        src/light_buffer.cpp
        src/combined_buffer.cpp
        src/combined_buffer_pool.cpp
        src/debug_primitive.cpp
        src/vulkan_context.cpp
        src/gpu_timer.cpp
        src/shadow_map.cpp
        src/cull_pipeline.cpp
        src/renderer.cpp
        src/debug_window.cpp
        src/frame_recorder.cpp
        src/engine.cpp
)

target_link_libraries(vw_gfx
    PUBLIC vw_core vw_ecs vw_world vw_platform VulkanHppModule
    PRIVATE glfw imgui::imgui)
target_compile_features(vw_gfx PUBLIC cxx_std_23)
vw_use_std_module(vw_gfx)
