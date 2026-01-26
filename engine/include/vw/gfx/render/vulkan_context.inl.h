#pragma once

#ifndef VW_GFX_VULKAN_CONTEXT_INL_H
#define VW_GFX_VULKAN_CONTEXT_INL_H

#include <cstring>
#include <set>
#include <stdexcept>

#include "vw/log.h"
#include "vw/gfx/window/window.h"

#ifndef NDEBUG
inline VkBool32 vw::gfx::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data
) {
    switch (message_severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            vw::log::error(
                vulkan_context::lc_, "Validation layer: {}", p_callback_data->pMessage
            );
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            vw::log::warn(
                vulkan_context::lc_, "Validation layer: {}", p_callback_data->pMessage
            );
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            vw::log::info(
                vw::gfx::vulkan_context::lc_, "Validation layer: {}", p_callback_data->pMessage
            );
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            vw::log::debug(
                vw::gfx::vulkan_context::lc_, "Validation layer: {}", p_callback_data->pMessage
            );
            break;
        default:
            vw::log::info(
                vw::gfx::vulkan_context::lc_, "Validation layer: {}", p_callback_data->pMessage
            );
            break;
    }
    return VK_FALSE;
}

inline VkResult create_debug_utils_messenger_ext(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger
) {
    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
    );
    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void destroy_debug_utils_messenger_ext(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* p_allocator
) {
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (func != nullptr) {
        func(instance, debug_messenger, p_allocator);
    }
}
#endif

namespace vw::gfx {
inline vulkan_context::vulkan_context(
    window& window
)
    : window_(&window) {
    device_extensions_ = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME};
#ifdef __APPLE__
    device_extensions_.push_back("VK_KHR_portability_subset");
#endif

    create_instance_();

#ifndef NDEBUG
    setup_debug_messenger_();
#endif

    create_surface_();
    pick_physical_device_();
    create_logical_device_();
    create_command_pool_();
}

inline vulkan_context::~vulkan_context() {
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    vkDestroyDevice(device_, nullptr);
#ifndef NDEBUG
    destroy_debug_utils_messenger_ext(instance_, debug_messenger_, nullptr);
#endif
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

inline VkInstance vulkan_context::get_instance() const {
    return instance_;
}

inline VkDevice vulkan_context::get_device() const {
    return device_;
}

inline VkPhysicalDevice vulkan_context::get_physical_device() const {
    return physical_device_;
}

inline VkQueue vulkan_context::get_graphics_queue() const {
    return graphics_queue_;
}

inline VkQueue vulkan_context::get_present_queue() const {
    return present_queue_;
}

inline VkSurfaceKHR vulkan_context::get_surface() const {
    return surface_;
}

inline VkCommandPool vulkan_context::get_command_pool() const {
    return command_pool_;
}

inline queue_family_indices vulkan_context::get_queue_families() const {
    return queue_families_;
}

inline swapchain_support_details vulkan_context::query_swapchain_support_() const {
    return query_swapchain_support_(physical_device_);
}

inline void vulkan_context::create_instance_() {
    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Voxel World";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "No Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_2;

    VkInstanceCreateInfo create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

#ifdef __APPLE__
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    auto extensions = window::get_required_extensions();

#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    vw::log::debug(lc_, "Requested instance extensions:");
    for (const auto& extension : extensions) {
        vw::log::debug(lc_, "\t- {}", extension);
    }

    create_info.enabledExtensionCount   = static_cast<uint32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

#ifdef NDEBUG
    const std::vector validation_layers = {"VK_LAYER_KHRONOS_validation"};

    // Проверяем доступность слоев валидации
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    vw::log::debug(lc_, "Requested validation layers:");
    for (const auto& layer : validation_layers) {
        bool layer_found = false;
        for (const auto& available_layer : available_layers) {
            if (strcmp(layer, available_layer.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (layer_found) {
            vw::log::debug(lc_, "\t- {}", layer);
        } else {
            vw::log::debug(lc_, "\t- {} - not found", layer);
        }
    }

    create_info.enabledLayerCount   = static_cast<uint32>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
#else
    create_info.enabledLayerCount = 0;
#endif

    // Создаем инстанс с детальной обработкой ошибок
    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    if (result != VK_SUCCESS) {
        std::string error_msg = "Failed to create Vulkan instance. Error code: ";

        switch (result) {
            case VK_ERROR_OUT_OF_HOST_MEMORY:
                error_msg += "VK_ERROR_OUT_OF_HOST_MEMORY";
                break;
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                error_msg += "VK_ERROR_OUT_OF_DEVICE_MEMORY";
                break;
            case VK_ERROR_INITIALIZATION_FAILED:
                error_msg += "VK_ERROR_INITIALIZATION_FAILED";
                break;
            case VK_ERROR_LAYER_NOT_PRESENT:
                error_msg += "VK_ERROR_LAYER_NOT_PRESENT";
                break;
            case VK_ERROR_EXTENSION_NOT_PRESENT:
                error_msg += "VK_ERROR_EXTENSION_NOT_PRESENT";
                break;
            case VK_ERROR_INCOMPATIBLE_DRIVER:
                error_msg += "VK_ERROR_INCOMPATIBLE_DRIVER";
                break;
            default:
                error_msg += "Unknown error (" + std::to_string(result) + ")";
                break;
        }

        vw::log::error(lc_, "{}", error_msg);
        throw std::runtime_error(error_msg);
    }
}

inline void vulkan_context::create_surface_() {
    surface_ = window_->create_surface(instance_);
}

inline void vulkan_context::pick_physical_device_() {
    uint32 device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

    if (device_count == 0) {
        throw std::runtime_error("failed to find gpu with vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    vw::log::info(lc_, "Found {} devices with vulkan support", device_count);

    for (const auto& device : devices) {
        if (is_device_suitable_(device)) {
            physical_device_ = device;
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            std::string name = props.deviceName;
            vw::log::info(lc_, "Selected device: {}", name);
            return;
        }
    }

    vw::log::error(lc_, "No suitable devices found!");
    throw std::runtime_error("failed to find a suitable gpu");
}

inline void vulkan_context::create_logical_device_() {
    queue_families_ = find_queue_families_(physical_device_);
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32> unique_queue_families = {
        queue_families_.graphics_family.value(), queue_families_.present_family.value()
    };

    float queue_priority = 1.0f;
    for (uint32 queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    device_features.fillModeNonSolid  = VK_TRUE;
    device_features.multiDrawIndirect = VK_TRUE;

    VkPhysicalDeviceShaderDrawParametersFeatures shader_draw_params{};
    shader_draw_params.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
    shader_draw_params.shaderDrawParameters = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.pNext = &shader_draw_params;

    VkDeviceCreateInfo create_info{};
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext                   = &features12;
    create_info.queueCreateInfoCount    = static_cast<uint32>(queue_create_infos.size());
    create_info.pQueueCreateInfos       = queue_create_infos.data();
    create_info.pEnabledFeatures        = &device_features;
    create_info.enabledExtensionCount   = static_cast<uint32>(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();
    create_info.enabledLayerCount     = 0;

    if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device");
    }

    vkGetDeviceQueue(device_, queue_families_.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, queue_families_.present_family.value(), 0, &present_queue_);
}

inline void vulkan_context::create_command_pool_() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_families_.graphics_family.value();
    pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool");
    }
}

inline bool vulkan_context::is_device_suitable_(
    VkPhysicalDevice device
) {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    std::string device_name{device_properties.deviceName};
    vw::log::debug(lc_, "Checking device: {}", device_name);
    const char* device_type_str = "UNKNOWN";
    switch (device_properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            device_type_str = "OTHER";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            device_type_str = "INTEGRATED_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            device_type_str = "DISCRETE_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            device_type_str = "VIRTUAL_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            device_type_str = "CPU";
            break;
        default:
            break;
    }
    vw::log::debug(lc_, "Type: {}", device_type_str);

    vw::log::debug(
        lc_,
        "API Version: {}.{}.{}",
        VK_VERSION_MAJOR(device_properties.apiVersion),
        VK_VERSION_MINOR(device_properties.apiVersion),
        VK_VERSION_PATCH(device_properties.apiVersion)
    );

    vw::log::debug(
        lc_,
        "Driver Version: {}.{}.{}",
        VK_VERSION_MAJOR(device_properties.driverVersion),
        VK_VERSION_MINOR(device_properties.driverVersion),
        VK_VERSION_PATCH(device_properties.driverVersion)
    );

    queue_family_indices indices = find_queue_families_(device);
    vw::log::debug(lc_, "Queue Support:");
    vw::log::debug(lc_, "  Graphics: {}", indices.graphics_family.has_value() ? "Yes" : "No");
    vw::log::debug(lc_, "  Present: {}", indices.present_family.has_value() ? "Yes" : "No");

    bool extensions_supported = check_device_extension_support_(device);
    vw::log::debug(lc_, "Extension Support: {}", extensions_supported ? "Yes" : "No");

    if (!extensions_supported) {
        vw::log::debug(lc_, "Required Extensions:");
        for (const auto& extension : device_extensions_) {
            vw::log::debug(lc_, "  - {}", extension);
        }

        uint32 extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(
            device, nullptr, &extension_count, available_extensions.data()
        );

        vw::log::debug(lc_, "Available Extensions:");
        for (const auto& extension : available_extensions) {
            vw::log::debug(lc_, "  - {}", extension.extensionName);
        }
    }

    bool swapchain_adequate = false;
    if (extensions_supported) {
        swapchain_support_details swapchain_support = query_swapchain_support_(device);
        swapchain_adequate =
            !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();

        vw::log::debug(lc_, "Swapchain Support: {}", swapchain_adequate ? "Yes" : "No");
        if (!swapchain_adequate) {
            vw::log::debug(lc_, "  Formats: {}", swapchain_support.formats.size());
            vw::log::debug(lc_, "  Present Modes: {}", swapchain_support.present_modes.size());
        }
    }

    VkBool32 surface_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        device, indices.graphics_family.value(), surface_, &surface_support
    );
    vw::log::debug(lc_, "Surface Support: {}", surface_support ? "Yes" : "No");

    bool suitable = indices.is_complete() && extensions_supported && swapchain_adequate;
    vw::log::debug(lc_, "Suitable: {}", suitable ? "Yes" : "No");

    return suitable;
}

inline queue_family_indices vulkan_context::find_queue_families_(
    VkPhysicalDevice device
) {
    queue_family_indices indices;
    uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present_support);
        if (present_support) {
            indices.present_family = i;
        }

        if (indices.is_complete()) {
            break;
        }
        i++;
    }

    return indices;
}

inline bool vulkan_context::check_device_extension_support_(
    VkPhysicalDevice device
) {
    uint32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(
        device, nullptr, &extension_count, available_extensions.data()
    );

    std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    if (!required_extensions.empty()) {
        vw::log::debug(lc_, "Missing extensions:");
        for (const auto& missing : required_extensions) {
            vw::log::debug(lc_, "  - {}", missing);
        }
    }

    return required_extensions.empty();
}

inline swapchain_support_details vulkan_context::query_swapchain_support_(
    VkPhysicalDevice device
) const {
    swapchain_support_details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, surface_, &format_count, details.formats.data()
        );
    }

    uint32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface_, &present_mode_count, details.present_modes.data()
        );
    }

    return details;
}

#ifndef NDEBUG
inline void vulkan_context::setup_debug_messenger_() {
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;

    if (create_debug_utils_messenger_ext(instance_, &create_info, nullptr, &debug_messenger_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger");
    }
}
#endif

}  // namespace vw::gfx

#endif  // VW_GFX_VULKAN_CONTEXT_INL_H
