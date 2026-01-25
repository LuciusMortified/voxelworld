#pragma once

#ifndef VW_GFX_RENDER_SHADOW_MAP_H
#define VW_GFX_RENDER_SHADOW_MAP_H

#include <vulkan/vulkan.h>

#include <array>
#include <memory>

#include "vw/core.h"
#include "vw/gfx/spatial/frustum.h"

namespace vw::gfx {

class vulkan_context;
class camera;

class shadow_map {
public:
    static constexpr uint32 cascade_count   = 4;
    static constexpr uint32 shadow_map_size = 4192;

    explicit shadow_map(vulkan_context& context);
    ~shadow_map();

    shadow_map(const shadow_map&)            = delete;
    shadow_map& operator=(const shadow_map&) = delete;
    shadow_map(shadow_map&&)                 = delete;
    shadow_map& operator=(shadow_map&&)      = delete;

    void update(const camera& camera, const vec3f& light_direction);

    [[nodiscard]] mat4f get_light_space_matrix(uint32 cascade_index) const;
    [[nodiscard]] const std::array<mat4f, cascade_count>& get_light_space_matrices() const;
    [[nodiscard]] const std::array<float, cascade_count>& get_cascade_splits() const;
    [[nodiscard]] VkImage get_image() const;
    [[nodiscard]] VkImageView get_image_view(uint32 cascade_index) const;
    [[nodiscard]] VkImageView get_array_image_view() const;
    [[nodiscard]] VkSampler get_sampler() const;
    [[nodiscard]] VkSampler get_debug_sampler() const;
    [[nodiscard]] VkFramebuffer get_framebuffer(uint32 cascade_index) const;
    [[nodiscard]] VkRenderPass get_render_pass() const;

private:
    void create_shadow_map_image();
    void create_sampler();
    void create_framebuffers();
    void create_render_pass();
    void cleanup();

    vulkan_context* context_;

    VkImage shadow_image_                                              = VK_NULL_HANDLE;
    VkDeviceMemory shadow_image_memory_                                = VK_NULL_HANDLE;
    VkImageView shadow_array_image_view_                               = VK_NULL_HANDLE;
    std::array<VkImageView, cascade_count> shadow_cascade_image_views_ = {};
    VkSampler shadow_sampler_                                          = VK_NULL_HANDLE;
    VkSampler debug_sampler_                                           = VK_NULL_HANDLE;
    std::array<VkFramebuffer, cascade_count> shadow_framebuffers_      = {};
    VkRenderPass shadow_render_pass_                                   = VK_NULL_HANDLE;

    std::array<mat4f, cascade_count> light_space_matrices_ = {};
    std::array<float, cascade_count> cascade_splits_       = {};

    float split_lambda_        = 0.75f;
    float shadow_far_          = 500.f;
};

}  // namespace vw::gfx

#include "vw/gfx/render/shadow_map.inl.h"

#endif  // VW_GFX_RENDER_SHADOW_MAP_H
