#pragma once

#ifndef VW_GFX_RESOURCE_LIGHT_BUFFER_H
#define VW_GFX_RESOURCE_LIGHT_BUFFER_H

#include <vulkan/vulkan.h>

#include <memory>

#include "vw/core.h"
#include "vw/gfx/render/renderer.h"
#include "vw/gfx/resource/buffer.h"
#include "vw/ecs/world.h"

namespace vw::gfx {

using namespace ::vw::ecs;

class vulkan_context;

// Для point lights (в SSBO)
struct point_light_data {
    alignas(16) vec4f position;
    alignas(16) vec4f color;
    alignas(4) float32 intensity;
    alignas(4) float32 range;
    alignas(4) float32 attenuation_constant;
    alignas(4) float32 attenuation_linear;
    alignas(4) float32 attenuation_quadratic;
};

template <typename WC>
class light_buffer {
public:
    using world_type = world<WC>;

    explicit light_buffer(
        vulkan_context& context,
        VkDescriptorPool descriptor_pool,
        VkDescriptorSetLayout descriptor_set_layout
    );
    ~light_buffer();

    void update(world_type& world);

    [[nodiscard]] VkDescriptorSet get_descriptor_set() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] uint32 get_lights_count() const;

private:
    void expand_buffer_if_needed(uint32 required_count);
    void update_descriptor_set();

    static constexpr uint32_t default_capacity_ = 64;

    vulkan_context* context_;
    uint32 capacity_;
    std::unique_ptr<storage_buffer> lights_buffer_;

    VkDescriptorSet descriptor_set_              = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_            = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;

    uint32 lights_count_ = 0;
};

}  // namespace vw::gfx

#include "vw/gfx/resource/light_buffer.inl.h"

#endif  // VW_GFX_RESOURCE_LIGHT_BUFFER_H
