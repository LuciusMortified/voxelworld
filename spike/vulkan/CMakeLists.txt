find_package(Vulkan REQUIRED)

add_library(VulkanHppModule STATIC)

target_sources(VulkanHppModule
    PUBLIC
        FILE_SET CXX_MODULES BASE_DIRS ${Vulkan_INCLUDE_DIR} FILES
            ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)

target_compile_definitions(VulkanHppModule PUBLIC
    VULKAN_HPP_NO_EXCEPTIONS
    VULKAN_HPP_NO_CONSTRUCTORS
    VULKAN_HPP_NO_SMART_HANDLE
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
    VULKAN_HPP_CXX_MODULE_EXPERIMENTAL_WARNING
)

target_link_libraries(VulkanHppModule PUBLIC Vulkan::Headers vw_std)
target_compile_features(VulkanHppModule PUBLIC cxx_std_23)
