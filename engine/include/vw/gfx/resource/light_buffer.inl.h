#pragma once

#ifndef VW_GFX_RESOURCE_LIGHT_BUFFER_INL_H
#define VW_GFX_RESOURCE_LIGHT_BUFFER_INL_H

#include "vw/gfx/resource/light_buffer.h"
#include "vw/gfx/render/vulkan_context.h"
#include "vw/ecs/systems/light_system.h"
#include "vw/ecs/components/light_component.h"
#include "vw/ecs/components/transform_component.h"

namespace vw::gfx {

using namespace ::vw::ecs;

inline light_buffer::light_buffer(
    vulkan_context& context,
    VkDescriptorPool descriptor_pool,
    VkDescriptorSetLayout descriptor_set_layout
)
    : context_(&context)
    , capacity_(default_capacity_)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout)
    , lights_count_(0) {
    lights_buffer_ = std::make_unique<storage_buffer>(
        *context_, capacity_ * sizeof(point_light_data)
    );

    // Выделяем descriptor set из pool
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &descriptor_set_layout_;

    if (vkAllocateDescriptorSets(context_->get_device(), &alloc_info, &descriptor_set_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor set for light_buffer!");
    }

    update_descriptor_set();
}

inline light_buffer::~light_buffer() {
    if (descriptor_set_ != VK_NULL_HANDLE && descriptor_pool_ != VK_NULL_HANDLE) {
        vkFreeDescriptorSets(context_->get_device(), descriptor_pool_, 1, &descriptor_set_);
    }
}

inline void light_buffer::update(world_type& world) {
    auto& light_changed = world.changed<light_component>();
    if (light_changed.empty()) {
        return;
    }

    std::vector<point_light_data> point_lights_data;

    auto view = world.view<light_component, transform_component>();
    for (const auto& [ent, light_comp, transform_comp] : view) {
        point_light_data light_data;
        const vec3f& pos = transform_comp.get_position();
        light_data.position = vec4f(pos.x, pos.y, pos.z, 0.0f);
        const vec3f& col = light_comp.get_color();
        light_data.color = vec4f(col.x, col.y, col.z, 0.0f);
        light_data.intensity = light_comp.get_intensity();
        light_data.range = light_comp.get_range();
        light_data.attenuation_constant = light_comp.get_attenuation_constant();
        light_data.attenuation_linear = light_comp.get_attenuation_linear();
        light_data.attenuation_quadratic = light_comp.get_attenuation_quadratic();

        point_lights_data.push_back(light_data);
    }

    lights_count_ = static_cast<uint32>(point_lights_data.size());

    expand_buffer_if_needed(lights_count_);

    if (lights_count_ > 0) {
        lights_buffer_->copy_from_vector(point_lights_data);
    }
}

inline VkDescriptorSet light_buffer::get_descriptor_set() const {
    return descriptor_set_;
}

inline bool light_buffer::is_empty() const {
    return lights_count_ == 0;
}

inline uint32 light_buffer::get_lights_count() const {
    return lights_count_;
}

inline void light_buffer::expand_buffer_if_needed(uint32 required_count) {
    if (required_count <= capacity_) {
        return;
    }

    while (capacity_ < required_count) {
        capacity_ *= 2;
    }

    auto new_lights_buffer = std::make_unique<storage_buffer>(
        *context_, capacity_ * sizeof(point_light_data)
    );

    if (lights_count_ > 0) {
        lights_buffer_->copy_to_buffer(
            *new_lights_buffer, lights_count_ * sizeof(point_light_data)
        );
    }

    lights_buffer_ = std::move(new_lights_buffer);
    update_descriptor_set();
}

inline void light_buffer::update_descriptor_set() {
    VkDescriptorBufferInfo storage_buffer_info{};
    storage_buffer_info.buffer = lights_buffer_->get_buffer();
    storage_buffer_info.offset = 0;
    storage_buffer_info.range  = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptor_write{};
    descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.dstSet          = descriptor_set_;
    descriptor_write.dstBinding      = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo     = &storage_buffer_info;

    vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
}

}  // namespace vw::gfx

#endif  // VW_GFX_RESOURCE_LIGHT_BUFFER_INL_H
