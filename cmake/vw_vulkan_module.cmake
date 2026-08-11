# Provides the Vulkan-Hpp C++ module, validated by the M0 spike.
#
# Vulkan-Headers names the module `vulkan` since 1.4.334 and keeps the dynamic
# dispatcher storage inside it, so a consumer needs neither the C headers nor a
# storage macro -- `import vulkan;` and a no-argument `init()` are enough. The
# configuration macros must sit on this target: they change the module
# interface, and a consumer defining them textually would not agree with it.
#
# Nothing calls this yet; gfx picks it up when it becomes a module in M5.

function(vw_add_vulkan_module)
    if(TARGET VulkanHppModule)
        return()
    endif()

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

    target_link_libraries(VulkanHppModule PUBLIC Vulkan::Headers)
    target_compile_features(VulkanHppModule PUBLIC cxx_std_23)

    vw_use_std_module(VulkanHppModule)
endfunction()
