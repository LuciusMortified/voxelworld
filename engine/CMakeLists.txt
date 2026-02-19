cmake_minimum_required(VERSION 3.25)

project(vwengine)

add_library(${PROJECT_NAME} INTERFACE
        include/vw/gfx/world/components/model_component.inl.h
        include/vw/gfx/world/entity_archetype.h
        include/vw/gfx/world/entity_archetype.inl.h
        include/vw/gfx/world/entity_guard.h
        include/vw/gfx/world/entity_guard.inl.h
        include/vw/gfx/world/entity_guard_group.h
        include/vw/gfx/world/entity_guard_group.inl.h
        include/vw/gfx/world/serializers/vox_serializer.h
        include/vw/gfx/world/serializers/vox_serializer.inl.h
        include/vw/gfx/world/serializers/vox_deserializer.h
        include/vw/gfx/world/serializers/vox_deserializer.inl.h)

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} INTERFACE SPDLOG_USE_STD_FORMAT)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} INTERFACE
        ${Vulkan_LIBRARIES}
        glfw
        imgui::imgui
        spdlog::spdlog_header_only
)
