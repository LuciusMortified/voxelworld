#pragma once

#ifndef VW_GFX_RESOURCE_PALETTE_BUFFER_H
#define VW_GFX_RESOURCE_PALETTE_BUFFER_H

#include <vulkan/vulkan.h>

#include <memory>

#include "vw/core/block_registry.h"
#include "vw/gfx/resource/buffer.h"

namespace vw::gfx {

class vulkan_context;

class palette_buffer {
public:
    palette_buffer(
        vulkan_context& context,
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout,
        const block_registry& registry
    );
    ~palette_buffer();

    [[nodiscard]] auto get_descriptor_set() const -> VkDescriptorSet;

private:
    vulkan_context* context_;
    std::unique_ptr<storage_buffer> buffer_;
    VkDescriptorSet descriptor_set_              = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_            = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
};

}  // namespace vw::gfx

#include "vw/gfx/resource/palette_buffer.inl.h"

#endif  // VW_GFX_RESOURCE_PALETTE_BUFFER_H
