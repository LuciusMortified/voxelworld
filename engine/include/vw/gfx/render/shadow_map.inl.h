#pragma once

#ifndef VW_GFX_RENDER_SHADOW_MAP_INL_H
#define VW_GFX_RENDER_SHADOW_MAP_INL_H

#include <algorithm>
#include <limits>

#include "vw/core/math.h"
#include "vw/core/vec4.h"
#include "vw/gfx/camera/camera.h"
#include "vw/gfx/render/shadow_map.h"
#include "vw/gfx/render/vulkan_context.h"

namespace vw::gfx {

inline shadow_map::shadow_map(
    vulkan_context& context
)
    : context_(&context), light_space_matrix_(math::identity_matrix()) {
    create_shadow_map_image();
    create_sampler();
    create_render_pass();
    create_framebuffer();
}

inline shadow_map::~shadow_map() {
    cleanup();
}

inline void shadow_map::create_shadow_map_image() {
    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;

    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = shadow_map_size;
    image_info.extent.height = shadow_map_size;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.format        = depth_format;
    image_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage   = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context_->get_device(), &image_info, nullptr, &shadow_image_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(context_->get_device(), shadow_image_, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;

    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context_->get_physical_device(), &mem_properties);

    uint32 memory_type_index = UINT32_MAX;
    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((mem_requirements.memoryTypeBits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags &
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memory_type_index = i;
            break;
        }
    }

    if (memory_type_index == UINT32_MAX) {
        throw std::runtime_error("Failed to find suitable memory type for shadow map!");
    }

    alloc_info.memoryTypeIndex = memory_type_index;

    if (vkAllocateMemory(context_->get_device(), &alloc_info, nullptr, &shadow_image_memory_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate shadow map image memory!");
    }

    vkBindImageMemory(context_->get_device(), shadow_image_, shadow_image_memory_, 0);

    VkImageViewCreateInfo view_info{};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = shadow_image_;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = depth_format;
    view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    if (vkCreateImageView(context_->get_device(), &view_info, nullptr, &shadow_image_view_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map image view!");
    }
}

inline void shadow_map::create_sampler() {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter               = VK_FILTER_LINEAR;
    sampler_info.minFilter               = VK_FILTER_LINEAR;
    sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler_info.mipLodBias              = 0.0f;
    sampler_info.anisotropyEnable        = VK_FALSE;
    sampler_info.maxAnisotropy           = 1.0f;
    sampler_info.compareEnable           = VK_TRUE;
    sampler_info.compareOp               = VK_COMPARE_OP_LESS_OR_EQUAL;
    sampler_info.minLod                  = 0.0f;
    sampler_info.maxLod                  = 1.0f;
    sampler_info.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler_info.unnormalizedCoordinates = VK_FALSE;

    if (vkCreateSampler(context_->get_device(), &sampler_info, nullptr, &shadow_sampler_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map sampler!");
    }
}

inline void shadow_map::create_render_pass() {
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format         = VK_FORMAT_D32_SFLOAT;
    depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 0;
    depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 0;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &depth_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies   = &dependency;

    if (vkCreateRenderPass(
            context_->get_device(), &render_pass_info, nullptr, &shadow_render_pass_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map render pass!");
    }
}

inline void shadow_map::create_framebuffer() {
    VkFramebufferCreateInfo framebuffer_info{};
    framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_info.renderPass      = shadow_render_pass_;
    framebuffer_info.attachmentCount = 1;
    framebuffer_info.pAttachments    = &shadow_image_view_;
    framebuffer_info.width           = shadow_map_size;
    framebuffer_info.height          = shadow_map_size;
    framebuffer_info.layers          = 1;

    if (vkCreateFramebuffer(
            context_->get_device(), &framebuffer_info, nullptr, &shadow_framebuffer_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map framebuffer!");
    }
}

inline void shadow_map::update_light_matrix(
    const camera& camera, const vec3f& light_direction
) {
    const mat4f view_proj     = camera.get_view_projection_matrix();
    const mat4f inv_view_proj = math::inverse_matrix(view_proj);

    const std::array<vec4f, 8> ndc = {
        vec4f{-1.0f, -1.0f, -1.0f, 1.0f},
        vec4f{+1.0f, -1.0f, -1.0f, 1.0f},
        vec4f{+1.0f, +1.0f, -1.0f, 1.0f},
        vec4f{-1.0f, +1.0f, -1.0f, 1.0f},

        vec4f{-1.0f, -1.0f, +1.0f, 1.0f},
        vec4f{+1.0f, -1.0f, +1.0f, 1.0f},
        vec4f{+1.0f, +1.0f, +1.0f, 1.0f},
        vec4f{-1.0f, +1.0f, +1.0f, 1.0f},
    };

    std::array<vec3f, 8> frustum_ws{};
    vec3f center_ws{0.0f, 0.0f, 0.0f};

    for (size_t i = 0; i < ndc.size(); ++i) {
        const vec4f p     = inv_view_proj * ndc[i];
        const float inv_w = (p.w != 0.0f) ? (1.0f / p.w) : 1.0f;
        frustum_ws[i]     = vec3f{p.x * inv_w, p.y * inv_w, p.z * inv_w};
        center_ws         = center_ws + frustum_ws[i];
    }

    const vec3f light_dir = math::normalize(light_direction);

    vec3f light_up    = vec3f{0.0f, 1.0f, 0.0f};
    vec3f light_right = math::cross(light_up, light_dir);

    light_right = math::normalize(light_right);
    light_up    = math::normalize(math::cross(light_dir, light_right));


    vec3f light_target = camera.get_position();
    vec3f light_pos = light_target - light_dir * camera.get_far();
    mat4f light_view = math::look_at_matrix(light_pos, light_target, light_up);

    vec3f min_ls{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max()
    };
    vec3f max_ls{
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max()
    };

    for (const vec3f& p_ws : frustum_ws) {
        const vec4f p_ls4 = light_view * vec4f{p_ws.x, p_ws.y, p_ws.z, 1.0f};
        const vec3f p_ls{p_ls4.x, p_ls4.y, p_ls4.z};

        min_ls.x = std::min(min_ls.x, p_ls.x);
        min_ls.y = std::min(min_ls.y, p_ls.y);
        min_ls.z = std::min(min_ls.z, p_ls.z);

        max_ls.x = std::max(max_ls.x, p_ls.x);
        max_ls.y = std::max(max_ls.y, p_ls.y);
        max_ls.z = std::max(max_ls.z, p_ls.z);
    }

    constexpr float pad_xy = 10.0f;
    min_ls.x -= pad_xy;
    min_ls.y -= pad_xy;
    max_ls.x += pad_xy;
    max_ls.y += pad_xy;

    mat4f light_proj =
        math::orthographic_matrix(min_ls.x, max_ls.x, min_ls.y, max_ls.y, min_ls.z, max_ls.z);

    light_view = math::look_at_matrix(vec3f{0, 20, 0}, vec3f{0, 0, 0}, vec3f{1, 0, 0});
    light_proj = math::orthographic_matrix(-50, 50, -50, 50, 0.1f, 100.0f);

    light_space_matrix_ = light_proj * light_view;
}

inline mat4f shadow_map::get_light_space_matrix() const {
    return light_space_matrix_;
}

inline VkImageView shadow_map::get_image_view() const {
    return shadow_image_view_;
}

inline VkSampler shadow_map::get_sampler() const {
    return shadow_sampler_;
}

inline VkFramebuffer shadow_map::get_framebuffer() const {
    return shadow_framebuffer_;
}

inline VkRenderPass shadow_map::get_render_pass() const {
    return shadow_render_pass_;
}

inline void shadow_map::cleanup() {
    if (shadow_framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(context_->get_device(), shadow_framebuffer_, nullptr);
        shadow_framebuffer_ = VK_NULL_HANDLE;
    }
    if (shadow_render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(context_->get_device(), shadow_render_pass_, nullptr);
        shadow_render_pass_ = VK_NULL_HANDLE;
    }
    if (shadow_sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(context_->get_device(), shadow_sampler_, nullptr);
        shadow_sampler_ = VK_NULL_HANDLE;
    }
    if (shadow_image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(context_->get_device(), shadow_image_view_, nullptr);
        shadow_image_view_ = VK_NULL_HANDLE;
    }
    if (shadow_image_ != VK_NULL_HANDLE) {
        vkDestroyImage(context_->get_device(), shadow_image_, nullptr);
        shadow_image_ = VK_NULL_HANDLE;
    }
    if (shadow_image_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->get_device(), shadow_image_memory_, nullptr);
        shadow_image_memory_ = VK_NULL_HANDLE;
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_SHADOW_MAP_INL_H
