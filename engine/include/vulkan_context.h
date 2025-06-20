#pragma once
#include <vector>
#include <optional>

#include "types.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace voxel {
    class window;
    
    struct queue_family_indices {
        std::optional<uint32> graphics_family;
        std::optional<uint32> present_family;
        
        bool is_complete() const {
            return graphics_family.has_value() && present_family.has_value();
        }
    };
    
    struct swapchain_support_details {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    class vulkan_context {
    public:
        vulkan_context(window& window);
        ~vulkan_context();

        // Запретить копирование
        vulkan_context(const vulkan_context&) = delete;
        vulkan_context& operator=(const vulkan_context&) = delete;

        VkInstance get_instance() const { return instance_; }
        VkDevice get_device() const { return device_; }
        VkPhysicalDevice get_physical_device() const { return physical_device_; }
        VkQueue get_graphics_queue() const { return graphics_queue_; }
        VkQueue get_present_queue() const { return present_queue_; }
        VkSurfaceKHR get_surface() const { return surface_; }
        VkCommandPool get_command_pool() const { return command_pool_; }
        
        queue_family_indices get_queue_families() const { return queue_families_; }
        swapchain_support_details query_swapchain_support() const;

    private:
        void create_instance();
        void create_surface();
        void pick_physical_device();
        void create_logical_device();
        void create_command_pool();

        bool is_device_suitable(VkPhysicalDevice device);
        queue_family_indices find_queue_families(VkPhysicalDevice device);
        bool check_device_extension_support(VkPhysicalDevice device);
        swapchain_support_details query_swapchain_support(VkPhysicalDevice device);

        window& window_;
        VkInstance instance_;
        VkSurfaceKHR surface_;
        VkPhysicalDevice physical_device_;
        VkDevice device_;
        VkQueue graphics_queue_;
        VkQueue present_queue_;
        VkCommandPool command_pool_;
        queue_family_indices queue_families_;

        const std::vector<const char*> device_extensions_ = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

#ifdef DEBUG
        VkDebugUtilsMessengerEXT debug_messenger_;
        void setup_debug_messenger();
#endif
    };
}