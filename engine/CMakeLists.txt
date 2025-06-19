project(voxelengine)

add_library(${PROJECT_NAME} STATIC
    src/engine.cpp
    src/model.cpp
    src/world.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC include)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan glfw glm) 