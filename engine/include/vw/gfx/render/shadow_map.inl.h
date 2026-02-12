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
    : context_(&context) {
    // Инициализируем матрицы единичными
    for (auto& matrix : light_space_matrices_) {
        matrix = math::identity_matrix();
    }
    create_shadow_map_image();
    create_sampler();
    create_render_pass();
    create_framebuffers();
}

inline shadow_map::~shadow_map() {
    cleanup();
}

inline void shadow_map::create_shadow_map_image() {
    constexpr std::array candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    VkFormat depth_format = VK_FORMAT_UNDEFINED;
    for (VkFormat fmt : candidates) {
        VkFormatProperties props{};
        vkGetPhysicalDeviceFormatProperties(context_->get_physical_device(), fmt, &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
            depth_format = fmt;
            break;
        }
    }
    if (depth_format == VK_FORMAT_UNDEFINED) {
        throw std::runtime_error("Failed to find supported depth format for shadow map!");
    }

    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = shadow_map_size;
    image_info.extent.height = shadow_map_size;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = cascade_count;
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

    // Создаем array view для использования в шейдерах
    VkImageViewCreateInfo array_view_info{};
    array_view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    array_view_info.image                           = shadow_image_;
    array_view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    array_view_info.format                          = depth_format;
    array_view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    array_view_info.subresourceRange.baseMipLevel   = 0;
    array_view_info.subresourceRange.levelCount     = 1;
    array_view_info.subresourceRange.baseArrayLayer = 0;
    array_view_info.subresourceRange.layerCount     = cascade_count;

    if (vkCreateImageView(
            context_->get_device(), &array_view_info, nullptr, &shadow_array_image_view_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shadow map array image view!");
    }

    // Создаем отдельные 2D views для каждого каскада (для debug отображения и framebuffers)
    for (uint32 i = 0; i < cascade_count; ++i) {
        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = shadow_image_;
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = depth_format;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = i;
        view_info.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(
                context_->get_device(), &view_info, nullptr, &shadow_cascade_image_views_[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map cascade image view!");
        }
    }
}

inline void shadow_map::create_sampler() {
    VkSamplerCreateInfo sampler_info{};
    sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter               = VK_FILTER_NEAREST;
    sampler_info.minFilter               = VK_FILTER_NEAREST;
    sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
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

    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp     = VK_COMPARE_OP_NEVER;

    if (vkCreateSampler(context_->get_device(), &sampler_info, nullptr, &debug_sampler_) !=
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

inline void shadow_map::create_framebuffers() {
    for (uint32 i = 0; i < cascade_count; ++i) {
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = shadow_render_pass_;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments    = &shadow_cascade_image_views_[i];
        framebuffer_info.width           = shadow_map_size;
        framebuffer_info.height          = shadow_map_size;
        framebuffer_info.layers          = 1;

        if (vkCreateFramebuffer(
                context_->get_device(), &framebuffer_info, nullptr, &shadow_framebuffers_[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shadow map framebuffer!");
        }
    }
}

inline void shadow_map::update(
    const camera& camera, const vec3f& light_direction
) {
    const vec3f light_dir = math::normalize(light_direction);

    // Вычисляем split distances используя Practical Split Scheme
    const float cam_near    = camera.get_near();
    const float cam_far     = camera.get_far();
    const float shadow_far  = std::min(shadow_far_, cam_far);
    const float shadow_dist = shadow_far - cam_near;

    for (uint32 i = 0; i < cascade_count; ++i) {
        float p            = static_cast<float>(i + 1) / static_cast<float>(cascade_count);
        float log          = cam_near * std::pow(shadow_far / cam_near, p);
        float uniform      = cam_near + shadow_dist * p;
        float d            = split_lambda_ * (log - uniform) + uniform;
        cascade_splits_[i] = d - cam_near;
    }

    // Вычисляем inverse view-projection матрицу камеры
    auto cam_proj = math::perspective_matrix(
        camera.get_fov(), camera.get_aspect_ratio(), cam_near, shadow_far
    );
    auto cam_view = camera.get_view_matrix();
    auto inv_result = math::inverse_matrix(cam_proj * cam_view);
    auto inv_cam = inv_result.value_or(math::identity_matrix());

    // Для каждого каскада вычисляем light space matrix
    float last_cascade_split = 0.0f;
    for (uint32 cascade_index = 0; cascade_index < cascade_count; ++cascade_index) {
        const float cascade_split = cascade_splits_[cascade_index];

        std::array frustum_corners = {
            vec3f{-1.0f, 1.0f, 0.0f},
            vec3f{1.0f, 1.0f, 0.0f},
            vec3f{1.0f, -1.0f, 0.0f},
            vec3f{-1.0f, -1.0f, 0.0f},
            vec3f{-1.0f, 1.0f, 1.0f},
            vec3f{1.0f, 1.0f, 1.0f},
            vec3f{1.0f, -1.0f, 1.0f},
            vec3f{-1.0f, -1.0f, 1.0f},
        };

        for (auto& corner : frustum_corners) {
            vec4f corner_homogeneous = vec4f{corner.x, corner.y, corner.z, 1.0f};
            vec4f corner_world       = inv_cam * corner_homogeneous;
            corner_world             = corner_world * (1.f / corner_world.w);
            corner                   = vec3f{corner_world.x, corner_world.y, corner_world.z};
        }

        for (int i = 0; i < 4; ++i) {
            vec3 dist              = frustum_corners[i + 4] - frustum_corners[i];
            frustum_corners[i + 4] = frustum_corners[i] + dist * (cascade_split / shadow_dist);
            frustum_corners[i]     = frustum_corners[i] + dist * (last_cascade_split / shadow_dist);
        }

        vec3 frustum_center = vec3f{0.0f, 0.0f, 0.0f};
        for (const auto& corner : frustum_corners) {
            frustum_center += corner;
        }
        frustum_center = frustum_center * (1.0f / static_cast<float>(frustum_corners.size()));

        float radius = 0.0f;
        for (const auto& corner : frustum_corners) {
            float distance = math::length(corner - frustum_center);
            radius         = std::max(radius, distance);
        }

        const vec3f max_extents = vec3f{radius, radius, radius};
        const vec3f min_extents = -max_extents;

        vec3f target = frustum_center;
        vec3f eye    = target - light_dir * shadow_dist;

        auto up = vec3f{0.0f, 1.0f, 0.0f};
        if (std::abs(math::dot(up, light_dir)) > 0.99f) {
            up = vec3f{1.0f, 0.0f, 0.0f};
        }

        const mat4f light_view = math::look_at_matrix(eye, target, up);

        auto light_proj = math::orthographic_matrix(
            min_extents.x, max_extents.x, min_extents.y, max_extents.y, cam_near, shadow_far
        );

        light_space_matrices_[cascade_index] = light_proj * light_view;

        last_cascade_split = cascade_split;
    }
}

inline mat4f shadow_map::get_light_space_matrix(
    uint32 cascade_index
) const {
    return light_space_matrices_[cascade_index];
}

inline const std::array<mat4f, shadow_map::cascade_count>& shadow_map::
    get_light_space_matrices() const {
    return light_space_matrices_;
}

inline const std::array<float, shadow_map::cascade_count>& shadow_map::get_cascade_splits() const {
    return cascade_splits_;
}

inline VkImage shadow_map::get_image() const {
    return shadow_image_;
}

inline VkImageView shadow_map::get_image_view(
    uint32 cascade_index
) const {
    return shadow_cascade_image_views_[cascade_index];
}

inline VkImageView shadow_map::get_array_image_view() const {
    return shadow_array_image_view_;
}

inline VkSampler shadow_map::get_sampler() const {
    return shadow_sampler_;
}

inline VkSampler shadow_map::get_debug_sampler() const {
    return debug_sampler_;
}

inline VkFramebuffer shadow_map::get_framebuffer(
    uint32 cascade_index
) const {
    return shadow_framebuffers_[cascade_index];
}

inline VkRenderPass shadow_map::get_render_pass() const {
    return shadow_render_pass_;
}

inline void shadow_map::cleanup() {
    for (uint32 i = 0; i < cascade_count; ++i) {
        if (shadow_framebuffers_[i] != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(context_->get_device(), shadow_framebuffers_[i], nullptr);
            shadow_framebuffers_[i] = VK_NULL_HANDLE;
        }
        if (shadow_cascade_image_views_[i] != VK_NULL_HANDLE) {
            vkDestroyImageView(context_->get_device(), shadow_cascade_image_views_[i], nullptr);
            shadow_cascade_image_views_[i] = VK_NULL_HANDLE;
        }
    }
    if (shadow_array_image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(context_->get_device(), shadow_array_image_view_, nullptr);
        shadow_array_image_view_ = VK_NULL_HANDLE;
    }
    if (shadow_render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(context_->get_device(), shadow_render_pass_, nullptr);
        shadow_render_pass_ = VK_NULL_HANDLE;
    }
    if (shadow_sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(context_->get_device(), shadow_sampler_, nullptr);
        shadow_sampler_ = VK_NULL_HANDLE;
    }
    if (debug_sampler_ != VK_NULL_HANDLE) {
        vkDestroySampler(context_->get_device(), debug_sampler_, nullptr);
        debug_sampler_ = VK_NULL_HANDLE;
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
