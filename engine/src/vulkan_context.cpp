#include <stdexcept>
#include <iostream>
#include <set>

#include <voxel/vulkan_context.h>
#include <voxel/window.h>
#include <voxel/types.h>

#ifdef DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data) {
    
    std::cerr << "Validation layer: " << p_callback_data->pMessage << std::endl;
    return VK_FALSE;
}

VkResult create_debug_utils_messenger_ext(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, 
    const VkAllocationCallbacks* p_allocator, 
    VkDebugUtilsMessengerEXT* p_debug_messenger) {
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroy_debug_utils_messenger_ext(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debug_messenger, 
    const VkAllocationCallbacks* p_allocator) {
    
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, p_allocator);
    }
}
#endif

namespace voxel {

vulkan_context::vulkan_context(std::shared_ptr<window> window) : window_(window) {

    device_extensions_ = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
        "VK_KHR_portability_subset",
#endif
    };

    create_instance();
#ifdef DEBUG
    setup_debug_messenger();
#endif
    create_surface();
    pick_physical_device();
    create_logical_device();
    create_command_pool();
}

vulkan_context::~vulkan_context() {
    vkDestroyCommandPool(device_, command_pool_, nullptr);
    vkDestroyDevice(device_, nullptr);
#ifdef DEBUG
    destroy_debug_utils_messenger_ext(instance_, debug_messenger_, nullptr);
#endif
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
}

// TODO: Реализовать методы
swapchain_support_details vulkan_context::query_swapchain_support() const {
    return query_swapchain_support(physical_device_);
}

void vulkan_context::create_instance() {
    // Информация о приложении
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Voxel World";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // Информация для создания инстанса
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

#ifdef __APPLE__
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Получаем необходимые расширения от GLFW
    auto extensions = window_->get_required_extensions();
    
    // Добавляем расширение для debug utils в DEBUG режиме
#ifdef DEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    
    std::cout << "Запрашиваемые расширения инстанса:" << std::endl;
    for (const auto& extension : extensions) {
        std::cout << "  - " << extension << std::endl;
    }
    
    create_info.enabledExtensionCount = static_cast<uint32>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    // Включаем слои валидации в DEBUG режиме
#ifdef DEBUG
    const std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};
    
    // Проверяем доступность слоев валидации
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
    
    std::cout << "Доступные слои валидации:" << std::endl;
    for (const auto& layer : available_layers) {
        std::cout << "  - " << layer.layerName << std::endl;
    }
    
    std::cout << "Запрашиваемые слои валидации:" << std::endl;
    for (const auto& layer : validation_layers) {
        bool layer_found = false;
        for (const auto& available_layer : available_layers) {
            if (strcmp(layer, available_layer.layerName) == 0) {
                layer_found = true;
                break;
            }
        }
        if (layer_found) {
            std::cout << "  ✓ " << layer << std::endl;
        } else {
            std::cout << "  ✗ " << layer << " (не найден)" << std::endl;
        }
    }
    
    create_info.enabledLayerCount = static_cast<uint32>(validation_layers.size());
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
        
        std::cerr << error_msg << std::endl;
        throw std::runtime_error(error_msg);
    }
    
    std::cout << "Vulkan instance создан успешно" << std::endl;
}

void vulkan_context::create_surface() {
    surface_ = window_->create_surface(instance_);
}

void vulkan_context::pick_physical_device() {
    uint32 device_count = 0;
    vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);

    if (device_count == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());

    std::cout << "Найдено " << device_count << " устройств с поддержкой Vulkan:" << std::endl;
    std::cout << "==========================================" << std::endl;

    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            physical_device_ = device;
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            std::cout << "Выбрано устройство: " << props.deviceName << std::endl;
            return;
        }
    }

    std::cout << "==========================================" << std::endl;
    std::cout << "Не найдено подходящих устройств!" << std::endl;
    throw std::runtime_error("Failed to find a suitable GPU");
}

void vulkan_context::create_logical_device() {
    queue_families_ = find_queue_families(physical_device_);
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32> unique_queue_families = {
        queue_families_.graphics_family.value(),
        queue_families_.present_family.value()
    };

    float queue_priority = 1.0f;
    for (uint32 queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32>(device_extensions_.size());
    create_info.ppEnabledExtensionNames = device_extensions_.data();

#ifdef DEBUG
    const std::vector<const char*> validation_layers = {"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = static_cast<uint32>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
#else
    create_info.enabledLayerCount = 0;
#endif

    if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }

    vkGetDeviceQueue(device_, queue_families_.graphics_family.value(), 0, &graphics_queue_);
    vkGetDeviceQueue(device_, queue_families_.present_family.value(), 0, &present_queue_);
}

void vulkan_context::create_command_pool() {
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = queue_families_.graphics_family.value();
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

bool vulkan_context::is_device_suitable(VkPhysicalDevice device) {
    // Получаем свойства устройства
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);
    
    std::cout << "Проверяем устройство: " << device_properties.deviceName << std::endl;
    std::cout << "  Тип: ";
    switch (device_properties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            std::cout << "OTHER";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            std::cout << "INTEGRATED_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            std::cout << "DISCRETE_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            std::cout << "VIRTUAL_GPU";
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            std::cout << "CPU";
            break;
        default:
            std::cout << "UNKNOWN";
            break;
    }
    std::cout << std::endl;
    
    std::cout << "  Версия API: " << VK_VERSION_MAJOR(device_properties.apiVersion) 
              << "." << VK_VERSION_MINOR(device_properties.apiVersion) 
              << "." << VK_VERSION_PATCH(device_properties.apiVersion) << std::endl;
    
    std::cout << "  Версия драйвера: " << VK_VERSION_MAJOR(device_properties.driverVersion) 
              << "." << VK_VERSION_MINOR(device_properties.driverVersion) 
              << "." << VK_VERSION_PATCH(device_properties.driverVersion) << std::endl;
    
    // Проверяем поддержку очередей
    queue_family_indices indices = find_queue_families(device);
    std::cout << "  Поддержка очередей:" << std::endl;
    std::cout << "    Graphics: " << (indices.graphics_family.has_value() ? "✓" : "✗") << std::endl;
    std::cout << "    Present: " << (indices.present_family.has_value() ? "✓" : "✗") << std::endl;
    
    // Проверяем поддержку расширений
    bool extensions_supported = check_device_extension_support(device);
    std::cout << "  Поддержка расширений: " << (extensions_supported ? "✓" : "✗") << std::endl;
    
    if (!extensions_supported) {
        std::cout << "    Требуемые расширения:" << std::endl;
        for (const auto& extension : device_extensions_) {
            std::cout << "      - " << extension << std::endl;
        }
        
        // Показываем доступные расширения
        uint32 extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
        
        std::cout << "    Доступные расширения:" << std::endl;
        for (const auto& extension : available_extensions) {
            std::cout << "      - " << extension.extensionName << std::endl;
        }
    }
    
    // Проверяем поддержку swapchain
    bool swapchain_adequate = false;
    if (extensions_supported) {
        swapchain_support_details swapchain_support = query_swapchain_support(device);
        swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
        
        std::cout << "  Поддержка swapchain: " << (swapchain_adequate ? "✓" : "✗") << std::endl;
        if (!swapchain_adequate) {
            std::cout << "    Форматы: " << swapchain_support.formats.size() << std::endl;
            std::cout << "    Режимы представления: " << swapchain_support.present_modes.size() << std::endl;
        }
    }
    
    // Проверяем поддержку поверхностей
    VkBool32 surface_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.graphics_family.value(), surface_, &surface_support);
    std::cout << "  Поддержка поверхности: " << (surface_support ? "✓" : "✗") << std::endl;
    
    // Итоговая проверка
    bool suitable = indices.is_complete() && extensions_supported && swapchain_adequate;
    std::cout << "  Итоговая оценка: " << (suitable ? "✓ ПОДХОДИТ" : "✗ НЕ ПОДХОДИТ") << std::endl;
    std::cout << std::endl;
    
    return suitable;
}

queue_family_indices vulkan_context::find_queue_families(VkPhysicalDevice device) {
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

bool vulkan_context::check_device_extension_support(VkPhysicalDevice device) {
    uint32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(device_extensions_.begin(), device_extensions_.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    if (!required_extensions.empty()) {
        std::cout << "    Отсутствующие расширения:" << std::endl;
        for (const auto& missing : required_extensions) {
            std::cout << "      - " << missing << std::endl;
        }
    }

    return required_extensions.empty();
}

swapchain_support_details vulkan_context::query_swapchain_support(VkPhysicalDevice device) const {
    swapchain_support_details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats.data());
    }

    uint32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes.data());
    }

    return details;
}

#ifdef DEBUG
void vulkan_context::setup_debug_messenger() {
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = 
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    
    if (create_debug_utils_messenger_ext(instance_, &create_info, nullptr, &debug_messenger_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up debug messenger");
    }
}
#endif

} // namespace voxel 