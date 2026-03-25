#pragma once

#ifndef VW_GFX_RENDER_CULL_PIPELINE_INL_H
#define VW_GFX_RENDER_CULL_PIPELINE_INL_H

#include <cstring>

#include "vw/gfx/render/vulkan_context.h"

namespace vw::gfx {

inline cull_pipeline::cull_pipeline(
    vulkan_context& context,
    VkDescriptorPool descriptor_pool
)
    : context_(&context)
    , descriptor_pool_(descriptor_pool) {
    compute_shader_ = std::make_unique<shader>(
        *context_, "shaders/cull.comp.spv", shader_type::COMPUTE
    );

    create_descriptor_set_layouts_();
    create_frustum_ubos_();
    create_pipeline_();
}

inline cull_pipeline::~cull_pipeline() {
    VkDevice device = context_->get_device();

    if (compute_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, compute_pipeline_, nullptr);
    }
    if (compute_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, compute_pipeline_layout_, nullptr);
    }
    if (frustum_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, frustum_descriptor_set_layout_, nullptr);
    }
    if (buffer_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, buffer_descriptor_set_layout_, nullptr);
    }
}

inline void cull_pipeline::create_descriptor_set_layouts_() {
    VkDescriptorSetLayoutBinding frustum_binding{};
    frustum_binding.binding         = 0;
    frustum_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    frustum_binding.descriptorCount = 1;
    frustum_binding.stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo frustum_layout_info{};
    frustum_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    frustum_layout_info.bindingCount = 1;
    frustum_layout_info.pBindings    = &frustum_binding;

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &frustum_layout_info, nullptr,
            &frustum_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create frustum descriptor set layout!");
    }

    std::array<VkDescriptorSetLayoutBinding, 4> buffer_bindings{};
    for (uint32 i = 0; i < 4; i++) {
        buffer_bindings[i].binding         = i;
        buffer_bindings[i].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        buffer_bindings[i].descriptorCount = 1;
        buffer_bindings[i].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
    }

    VkDescriptorSetLayoutCreateInfo buffer_layout_info{};
    buffer_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    buffer_layout_info.bindingCount = buffer_bindings.size();
    buffer_layout_info.pBindings    = buffer_bindings.data();

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &buffer_layout_info, nullptr,
            &buffer_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer descriptor set layout!");
    }
}

inline void cull_pipeline::create_pipeline_() {
    std::array<VkDescriptorSetLayout, 2> set_layouts{
        frustum_descriptor_set_layout_,
        buffer_descriptor_set_layout_,
    };

    VkPushConstantRange push_constant{};
    push_constant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push_constant.offset     = 0;
    push_constant.size       = sizeof(uint32);

    VkPipelineLayoutCreateInfo layout_info{};
    layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount         = set_layouts.size();
    layout_info.pSetLayouts            = set_layouts.data();
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges    = &push_constant;

    if (vkCreatePipelineLayout(
            context_->get_device(), &layout_info, nullptr, &compute_pipeline_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo pipeline_info{};
    pipeline_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.stage  = compute_shader_->get_stage_info();
    pipeline_info.layout = compute_pipeline_layout_;

    if (vkCreateComputePipelines(
            context_->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
            &compute_pipeline_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create compute pipeline!");
    }
}

inline void cull_pipeline::create_frustum_ubos_() {
    for (uint32 i = 0; i < max_frames_in_flight; i++) {
        frustum_ubos_[i] = std::make_unique<uniform_buffer>(
            *context_, static_cast<VkDeviceSize>(sizeof(cull_frustum_ubo))
        );
    }

    std::array<VkDescriptorSetLayout, max_frames_in_flight> layouts{};
    layouts.fill(frustum_descriptor_set_layout_);

    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = max_frames_in_flight;
    alloc_info.pSetLayouts        = layouts.data();

    if (vkAllocateDescriptorSets(
            context_->get_device(), &alloc_info, frustum_descriptor_sets_.data()
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate frustum descriptor sets!");
    }

    for (uint32 i = 0; i < max_frames_in_flight; i++) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = frustum_ubos_[i]->get_buffer();
        buffer_info.offset = 0;
        buffer_info.range  = sizeof(cull_frustum_ubo);

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = frustum_descriptor_sets_[i];
        write.dstBinding      = 0;
        write.dstArrayElement = 0;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(context_->get_device(), 1, &write, 0, nullptr);
    }
}

inline void cull_pipeline::update_frustums(
    uint32 frame_index,
    const frustum& view_frustum,
    std::span<const frustum> shadow_frustums
) {
    cull_frustum_ubo ubo{};
    ubo.pass_count = 1 + static_cast<uint32>(shadow_frustums.size());

    for (uint32 i = 0; i < 6; i++) {
        const auto& p = view_frustum.planes[i];
        ubo.planes[i] = vec4f{p.normal.x, p.normal.y, p.normal.z, p.distance};
    }

    for (uint32 c = 0; c < shadow_frustums.size(); c++) {
        for (uint32 i = 0; i < 6; i++) {
            const auto& p = shadow_frustums[c].planes[i];
            ubo.planes[(c + 1) * 6 + i] =
                vec4f{p.normal.x, p.normal.y, p.normal.z, p.distance};
        }
    }

    frustum_ubos_[frame_index]->copy_from_struct(ubo);
}

inline void cull_pipeline::dispatch(
    VkCommandBuffer cmd,
    combined_buffer& buffer,
    uint32 frame_index
) {
    const uint32 instance_count = buffer.get_draw_command_count();
    if (instance_count == 0) return;

    vkCmdFillBuffer(
        cmd,
        buffer.get_count_buffer(),
        0,
        combined_buffer::cull_pass_count * sizeof(uint32),
        0
    );

    VkMemoryBarrier fill_barrier{};
    fill_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    fill_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    fill_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0, 1, &fill_barrier, 0, nullptr, 0, nullptr
    );

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout_,
        0, 1, &frustum_descriptor_sets_[frame_index], 0, nullptr
    );

    VkDescriptorSet buffer_ds = buffer.get_compute_descriptor_set();
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout_,
        1, 1, &buffer_ds, 0, nullptr
    );

    vkCmdPushConstants(
        cmd, compute_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(uint32), &instance_count
    );

    const uint32 group_count = (instance_count + 63) / 64;
    vkCmdDispatch(cmd, group_count, 1, 1);
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_CULL_PIPELINE_INL_H
