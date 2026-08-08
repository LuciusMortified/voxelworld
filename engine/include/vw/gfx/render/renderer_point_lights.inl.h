#pragma once

#ifndef VW_GFX_RENDER_RENDERER_POINT_LIGHTS_INL_H
#define VW_GFX_RENDER_RENDERER_POINT_LIGHTS_INL_H

#include "vw/gfx/render/renderer.h"
#include "vw/gfx/render/vulkan_context.h"

namespace vw::gfx {

inline void renderer::create_point_lights_descriptor_set_layout() {
    // Point lights storage buffer descriptor set layout (set 3, binding 0)
    VkDescriptorSetLayoutBinding point_lights_layout_binding{};
    point_lights_layout_binding.binding            = 0;
    point_lights_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    point_lights_layout_binding.descriptorCount    = 1;
    point_lights_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    point_lights_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo point_lights_layout_info{};
    point_lights_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    point_lights_layout_info.bindingCount = 1;
    point_lights_layout_info.pBindings    = &point_lights_layout_binding;

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &point_lights_layout_info, nullptr, &point_lights_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create point lights descriptor set layout");
    }
}

inline void renderer::cleanup_point_lights_resources() {
    if (point_lights_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            context_->get_device(), point_lights_descriptor_set_layout_, nullptr
        );
        point_lights_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_RENDERER_POINT_LIGHTS_INL_H
