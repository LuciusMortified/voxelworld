#pragma once
#include <memory>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include "vw/types.h"

namespace vw::gfx {
class window;

struct queue_family_indices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> present_family;

    [[nodiscard]]
    bool is_complete() const {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct swapchain_support_details {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

class vulkan_context final {
public:
    explicit vulkan_context(window& window);
    ~vulkan_context();

    vulkan_context(const vulkan_context&) = delete;
    vulkan_context& operator=(const vulkan_context&) = delete;

    [[nodiscard]]
    VkInstance get_instance() const {
        return instance_;
    }

    [[nodiscard]]
    VkDevice get_device() const {
        return device_;
    }

    [[nodiscard]]
    VkPhysicalDevice get_physical_device() const {
        return physical_device_;
    }

    [[nodiscard]]
    VkQueue get_graphics_queue() const {
        return graphics_queue_;
    }

    [[nodiscard]]
    VkQueue get_present_queue() const {
        return present_queue_;
    }

    [[nodiscard]]
    VkSurfaceKHR get_surface() const {
        return surface_;
    }

    [[nodiscard]]
    VkCommandPool get_command_pool() const {
        return command_pool_;
    }

    [[nodiscard]]
    queue_family_indices get_queue_families() const {
        return queue_families_;
    }

    [[nodiscard]]
    swapchain_support_details query_swapchain_support() const;

private:
    void create_instance();
    void create_surface();
    void pick_physical_device();
    void create_logical_device();
    void create_command_pool();

    [[nodiscard]]
    bool is_device_suitable(VkPhysicalDevice device);

    [[nodiscard]]
    queue_family_indices find_queue_families(VkPhysicalDevice device);

    [[nodiscard]]
    bool check_device_extension_support(VkPhysicalDevice device);

    [[nodiscard]]
    swapchain_support_details query_swapchain_support(
        VkPhysicalDevice device
    ) const;

    window* window_;

    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    queue_family_indices queue_families_;

    std::vector<const char*> device_extensions_;

#ifdef DEBUG
    VkDebugUtilsMessengerEXT debug_messenger_;

    void setup_debug_messenger();
#endif
};
}  // namespace vw::gfx