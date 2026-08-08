cmake_minimum_required(VERSION 4.0)

project(vwengine)

add_subdirectory(core)
add_subdirectory(ecs)
add_subdirectory(world)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} INTERFACE
        vw_core
        vw_ecs
        vw_world
        ${Vulkan_LIBRARIES}
        glfw
        imgui::imgui
)
