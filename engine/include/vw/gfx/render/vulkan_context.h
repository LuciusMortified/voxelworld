#pragma once

#ifndef VW_GFX_VULKAN_CONTEXT_H
#define VW_GFX_VULKAN_CONTEXT_H

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

#include "vw/core.h"
#include "vw/log.h"
#include "vw/platform/window.h"

namespace vw::gfx {

using namespace ::vw::plat;

#ifndef NDEBUG
VkBool32 debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data
);
#endif

struct queue_family_indices {
    std::optional<uint32> graphics_family;
    std::optional<uint32> present_family;

    [[nodiscard]]
    bool is_complete() const {
        return graphics_family.has_value() && present_family.has_value();
    }
};

struct swapchain_support_details {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

class vulkan_context final {
public:
    explicit vulkan_context(window& window);
    ~vulkan_context();

    vulkan_context(const vulkan_context&)            = delete;
    vulkan_context& operator=(const vulkan_context&) = delete;

    [[nodiscard]] VkInstance get_instance() const;
    [[nodiscard]] VkDevice get_device() const;
    [[nodiscard]] VkPhysicalDevice get_physical_device() const;
    [[nodiscard]] VkQueue get_graphics_queue() const;
    [[nodiscard]] VkQueue get_present_queue() const;
    [[nodiscard]] VkSurfaceKHR get_surface() const;
    [[nodiscard]] VkCommandPool get_command_pool() const;
    [[nodiscard]] queue_family_indices get_queue_families() const;
    [[nodiscard]] swapchain_support_details query_swapchain_support_() const;

private:
    static constexpr log::log_category lc_{"vulkan_context"};

#ifndef NDEBUG
    friend VkBool32 debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT message_type,
        const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
        void* p_user_data
    );
#endif

    void create_instance_();
    void create_surface_();
    void pick_physical_device_();
    void create_logical_device_();
    void create_command_pool_();

    [[nodiscard]] bool is_device_suitable_(VkPhysicalDevice device);
    [[nodiscard]] queue_family_indices find_queue_families_(VkPhysicalDevice device);
    [[nodiscard]] bool check_device_extension_support_(VkPhysicalDevice device);
    [[nodiscard]] swapchain_support_details query_swapchain_support_(VkPhysicalDevice device) const;

    window* window_;

    VkInstance instance_              = VK_NULL_HANDLE;
    VkSurfaceKHR surface_             = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_                  = VK_NULL_HANDLE;
    VkQueue graphics_queue_           = VK_NULL_HANDLE;
    VkQueue present_queue_            = VK_NULL_HANDLE;
    VkCommandPool command_pool_       = VK_NULL_HANDLE;
    queue_family_indices queue_families_;

    std::vector<const char*> device_extensions_;

#ifndef NDEBUG
    VkDebugUtilsMessengerEXT debug_messenger_{};

    void setup_debug_messenger_();
#endif
};
}  // namespace vw::gfx

#include "vw/gfx/render/vulkan_context.inl.h"

#endif  // VW_GFX_VULKAN_CONTEXT_H
