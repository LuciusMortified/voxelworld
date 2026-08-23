find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_library(vw_platform STATIC)

target_sources(vw_platform
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS src FILES
            src/platform.cppm
            src/input/input.cppm
            src/input/event.cppm
            src/window/window.cppm
    PRIVATE
        src/window/window.cpp
)

target_include_directories(vw_platform SYSTEM PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vw_platform PUBLIC vw_core PRIVATE glfw)
target_compile_features(vw_platform PUBLIC cxx_std_23)
vw_use_std_module(vw_platform)
