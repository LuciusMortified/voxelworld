#pragma once

#ifndef VW_GFX_RESOURCE_PALETTE_BUFFER_INL_H
#define VW_GFX_RESOURCE_PALETTE_BUFFER_INL_H

#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/resource/palette_buffer.h"

namespace vw::gfx {

inline palette_buffer::palette_buffer(
    vulkan_context& context,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout,
    const block_registry& registry
)
    : context_(&context)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {
    std::array<uint32, 256> palette_data{};
    for (uint8 i = 0; i < registry.count(); ++i) {
        palette_data[i] = registry.get_color(i).value;
    }

    buffer_ = std::make_unique<storage_buffer>(*context_, sizeof(palette_data));
    buffer_->copy_from(palette_data.data(), sizeof(palette_data));

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &descriptor_set_layout_;

    if (vkAllocateDescriptorSets(context_->get_device(), &alloc_info, &descriptor_set_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set for palette_buffer!");
    }

    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = buffer_->get_buffer();
    buffer_info.offset = 0;
    buffer_info.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet          = descriptor_set_;
    write.dstBinding      = 0;
    write.dstArrayElement = 0;
    write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo     = &buffer_info;

    vkUpdateDescriptorSets(context_->get_device(), 1, &write, 0, nullptr);
}

inline palette_buffer::~palette_buffer() {
    if (descriptor_set_ != VK_NULL_HANDLE && descriptor_pool_ != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(context_->get_device(), descriptor_pool_, 1, &descriptor_set_);
    }
}

inline auto palette_buffer::get_descriptor_set() const -> VkDescriptorSet {
    return descriptor_set_;
}

}  // namespace vw::gfx

#endif  // VW_GFX_RESOURCE_PALETTE_BUFFER_INL_H
