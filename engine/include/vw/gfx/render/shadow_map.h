#pragma once

#ifndef VW_GFX_RENDER_SHADOW_MAP_H
#define VW_GFX_RENDER_SHADOW_MAP_H

#include <vulkan/vulkan.h>

#include <memory>

#include "vw/core.h"
#include "vw/gfx/spatial/frustum.h"

namespace vw::gfx {

class vulkan_context;
class camera;

class shadow_map {
public:
    explicit shadow_map(vulkan_context& context);
    ~shadow_map();

    shadow_map(const shadow_map&)            = delete;
    shadow_map& operator=(const shadow_map&) = delete;
    shadow_map(shadow_map&&)                 = delete;
    shadow_map& operator=(shadow_map&&)      = delete;

    void update_light_matrix(const camera& camera, const vec3f& light_direction);

    [[nodiscard]] mat4f get_light_space_matrix() const;

    [[nodiscard]] VkImageView get_image_view() const;
    [[nodiscard]] VkSampler get_sampler() const;
    [[nodiscard]] VkFramebuffer get_framebuffer() const;
    [[nodiscard]] VkRenderPass get_render_pass() const;

    static constexpr uint32 shadow_map_size = 2048;

private:
    void create_shadow_map_image();
    void create_sampler();
    void create_framebuffer();
    void create_render_pass();
    void cleanup();

    vulkan_context* context_;
    
    VkImage shadow_image_               = VK_NULL_HANDLE;
    VkDeviceMemory shadow_image_memory_ = VK_NULL_HANDLE;
    VkImageView shadow_image_view_      = VK_NULL_HANDLE;
    VkSampler shadow_sampler_           = VK_NULL_HANDLE;
    VkFramebuffer shadow_framebuffer_   = VK_NULL_HANDLE;
    VkRenderPass shadow_render_pass_    = VK_NULL_HANDLE;
    
    mat4f light_space_matrix_;

    float max_shadow_distance_ = 200.0f;
};

}  // namespace vw::gfx

#include "vw/gfx/render/shadow_map.inl.h"

#endif  // VW_GFX_RENDER_SHADOW_MAP_H
