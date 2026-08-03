cmake_minimum_required(VERSION 4.0)

project(vwengine)

add_subdirectory(core)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME} INTERFACE include)
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} INTERFACE SPDLOG_USE_STD_FORMAT)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} SYSTEM INTERFACE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME} INTERFACE
        vw_core
        ${Vulkan_LIBRARIES}
        glfw
        imgui::imgui
        spdlog::spdlog_header_only
)
