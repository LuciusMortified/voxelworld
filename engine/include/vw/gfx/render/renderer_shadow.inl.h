#pragma once

#ifndef VW_GFX_RENDER_RENDERER_SHADOW_INL_H
#define VW_GFX_RENDER_RENDERER_SHADOW_INL_H

#include <vulkan/vulkan.h>

#include "vw/gfx/render/renderer.h"
#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/resource/combined_buffer.h"
#include "vw/gfx/resource/mesh.h"

namespace vw::gfx {

template <typename C>
void renderer<C>::create_shadow_uniform_buffers() {
    VkDeviceSize buffer_size = sizeof(shadow_uniform_buffer_object);
    shadow_uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        shadow_uniform_buffers_[i] = std::make_unique<uniform_buffer>(*context_, buffer_size);
    }
}

template <typename C>
void renderer<C>::create_shadow_descriptor_sets() {
    std::vector layouts(MAX_FRAMES_IN_FLIGHT, uniform_descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts        = layouts.data();

    shadow_descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(
            context_->get_device(), &alloc_info, shadow_descriptor_sets_.data()
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate shadow descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo ubo_buffer_info{};
        ubo_buffer_info.buffer = shadow_uniform_buffers_[i]->get_buffer();
        ubo_buffer_info.offset = 0;
        ubo_buffer_info.range  = sizeof(shadow_uniform_buffer_object);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet          = shadow_descriptor_sets_[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo     = &ubo_buffer_info;

        vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
    }
}

template <typename C>
void renderer<C>::create_shadow_map_descriptor_sets() {
    std::vector layouts(MAX_FRAMES_IN_FLIGHT, shadow_descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts        = layouts.data();

    shadow_map_descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(
            context_->get_device(), &alloc_info, shadow_map_descriptor_sets_.data()
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate shadow map descriptor sets!");
    }

    // Обновляем descriptor sets с shadow map array image view и sampler
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo image_info{};
        image_info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        image_info.imageView   = shadow_map_->get_array_image_view();
        image_info.sampler     = shadow_map_->get_sampler();

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet          = shadow_map_descriptor_sets_[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pImageInfo      = &image_info;

        vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
    }
}

template <typename C>
void renderer<C>::create_shadow_pipeline() {
    // Используем shadow шейдеры
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        shadow_vertex_shader_->get_stage_info(), shadow_fragment_shader_->get_stage_info()
    };

    // Vertex input state (только позиция)
    auto binding_description    = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32_t>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    // Rasterizer state (для shadow mapping включаем depth bias)
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_FRONT_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.25f;
    rasterizer.depthBiasSlopeFactor    = 1.75f;
    rasterizer.depthBiasClamp          = 0.0f;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blend state (нет color attachments для shadow pass)
    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable   = VK_FALSE;
    color_blending.attachmentCount = 0;
    color_blending.pAttachments    = nullptr;

    // Depth stencil state
    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable  = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp   = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable     = VK_FALSE;

    VkPushConstantRange push_constant_range{};
    push_constant_range.offset     = 0;
    push_constant_range.size       = sizeof(shadow_push_constant_data);
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // Pipeline layout для shadow pass (uniform и storage descriptor set layouts)
    std::array shadow_descriptor_set_layouts = {
        uniform_descriptor_set_layout_, storage_descriptor_set_layout_
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount =
        static_cast<uint32_t>(shadow_descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts            = shadow_descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges    = &push_constant_range;

    if (vkCreatePipelineLayout(
            context_->get_device(), &pipeline_layout_info, nullptr, &shadow_pipeline_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow pipeline layout");
    }

    // Graphics pipeline для shadow pass
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipeline_info.stageCount          = 2;
    pipeline_info.pStages             = shader_stages;
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisampling;
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDepthStencilState  = &depth_stencil;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = shadow_pipeline_layout_;
    pipeline_info.renderPass          = shadow_map_->get_render_pass();
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(
            context_->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &shadow_pipeline_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow pipeline");
    }
}

template <typename WC>
void renderer<WC>::update_shadow_uniform_buffer() const {
    shadow_uniform_buffer_object ubo{};
    // Directional light data
    const auto& light_space_matrices = shadow_map_->get_light_space_matrices();
    for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
        ubo.light_space_matrices[i] = light_space_matrices[i];
    }
    shadow_uniform_buffers_[current_frame_]->copy_from_struct(ubo);
}

template <typename WC>
void renderer<WC>::render_shadow_pass(
    world_type& world, const camera& camera
) {
    update_shadow_uniform_buffer();

    // Рендерим каждый каскад отдельно
    for (uint32 cascade_index = 0; cascade_index < shadow_map::cascade_count; ++cascade_index) {
        // Начинаем shadow render pass для текущего каскада
        VkRenderPassBeginInfo render_pass_info{};
        render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass        = shadow_map_->get_render_pass();
        render_pass_info.framebuffer       = shadow_map_->get_framebuffer(cascade_index);
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = {
            shadow_map_->get_size(), shadow_map_->get_size()
        };

        VkClearValue clear_value{};
        clear_value.depthStencil = {1.0f, 0};

        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues    = &clear_value;

        vkCmdBeginRenderPass(
            command_buffers_[current_image_index_], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE
        );

        // Устанавливаем viewport и scissor для shadow map
        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(shadow_map_->get_size());
        viewport.height   = static_cast<float>(shadow_map_->get_size());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(command_buffers_[current_image_index_], 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {shadow_map_->get_size(), shadow_map_->get_size()};
        vkCmdSetScissor(command_buffers_[current_image_index_], 0, 1, &scissor);

        // Биндим shadow pipeline
        vkCmdBindPipeline(
            command_buffers_[current_image_index_],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadow_pipeline_
        );

        // Биндим shadow uniform buffer descriptor set (set 0)
        vkCmdBindDescriptorSets(
            command_buffers_[current_image_index_],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadow_pipeline_layout_,
            0,
            1,
            &shadow_descriptor_sets_[current_frame_],
            0,
            nullptr
        );

        // Устанавливаем push constant с индексом каскада
        shadow_push_constant_data push_constants{
            .cascade_index = cascade_index,
        };
        vkCmdPushConstants(
            command_buffers_[current_image_index_],
            shadow_pipeline_layout_,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(shadow_push_constant_data),
            &push_constants
        );

        // Рендерим все объекты из combined_buffer_pool
        const auto& buffers = combined_buffer_pool_->get_buffers();
        for (const auto& buffer : buffers) {
            if (buffer->is_empty()) {
                continue;
            }

            VkBuffer vertex_buffer         = buffer->get_vertex_buffer();
            VkBuffer instance_index_buffer = buffer->get_instance_index_buffer();
            VkBuffer index_buffer          = buffer->get_index_buffer();

            // Биндим storage buffer descriptor set из буфера (set 1)
            VkDescriptorSet buffer_descriptor_set = buffer->get_descriptor_set();
            vkCmdBindDescriptorSets(
                command_buffers_[current_image_index_],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                shadow_pipeline_layout_,
                1,  // Set index 1 (storage buffer descriptor set layout)
                1,
                &buffer_descriptor_set,
                0,
                nullptr
            );

            // Биндим vertex и index буферы
            constexpr VkDeviceSize vertex_offset   = 0;
            constexpr VkDeviceSize instance_offset = 0;

            std::array vertex_buffers = {vertex_buffer, instance_index_buffer};
            std::array vertex_offsets = {vertex_offset, instance_offset};

            vkCmdBindVertexBuffers(
                command_buffers_[current_image_index_],
                0,
                vertex_buffers.size(),
                vertex_buffers.data(),
                vertex_offsets.data()
            );
            vkCmdBindIndexBuffer(
                command_buffers_[current_image_index_], index_buffer, 0, VK_INDEX_TYPE_UINT32
            );

            const uint32_t max_draws  = buffer->get_draw_command_count();
            const uint32_t pass_index = cascade_index + 1;
            if (max_draws > 0) {
                vkCmdDrawIndexedIndirectCount(
                    command_buffers_[current_image_index_],
                    buffer->get_culled_indirect_buffer(),
                    static_cast<VkDeviceSize>(pass_index) * max_draws * sizeof(draw_command),
                    buffer->get_count_buffer(),
                    pass_index * sizeof(uint32_t),
                    max_draws,
                    sizeof(draw_command)
                );
            }
        }

        vkCmdEndRenderPass(command_buffers_[current_image_index_]);
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_RENDERER_SHADOW_INL_H
