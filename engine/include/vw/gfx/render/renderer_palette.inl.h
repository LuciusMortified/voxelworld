#pragma once

#ifndef VW_GFX_RENDER_RENDERER_PALETTE_INL_H
#define VW_GFX_RENDER_RENDERER_PALETTE_INL_H

#include "vw/gfx/render/renderer.h"
#include "vw/gfx/render/vulkan_context.h"

namespace vw::gfx {

inline void renderer::create_palette_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding palette_layout_binding{};
    palette_layout_binding.binding            = 0;
    palette_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    palette_layout_binding.descriptorCount    = 1;
    palette_layout_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    palette_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo palette_layout_info{};
    palette_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    palette_layout_info.bindingCount = 1;
    palette_layout_info.pBindings    = &palette_layout_binding;

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &palette_layout_info, nullptr,
            &palette_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create palette descriptor set layout");
    }
}

inline void renderer::cleanup_palette_resources() {
    if (palette_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            context_->get_device(), palette_descriptor_set_layout_, nullptr
        );
        palette_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_RENDERER_PALETTE_INL_H
