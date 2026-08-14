module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

using namespace ::vw::ecs;

light_buffer::light_buffer(
    vulkan_context& context,
    deletion_queue& deletion,
    vk::DescriptorPool descriptor_pool,
    vk::DescriptorSetLayout descriptor_set_layout
)
    : context_(&context)
    , deletion_(&deletion)
    , capacity_(default_capacity_)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout)
    , lights_count_(0) {
    lights_buffer_ = std::make_unique<storage_buffer>(
        *context_, capacity_ * sizeof(point_light_data)
    );

    descriptor_set_ = vk_must(
        context_->get_device().allocateDescriptorSets({
            .descriptorPool     = descriptor_pool_,
            .descriptorSetCount = 1,
            .pSetLayouts        = &descriptor_set_layout_,
        }),
        "allocate light descriptor set"
    ).front();

    update_descriptor_set();
}

light_buffer::~light_buffer() {
    if (descriptor_set_ && descriptor_pool_) {
        context_->get_device().freeDescriptorSets(descriptor_pool_, descriptor_set_);
    }
}

void light_buffer::update(world_type& world) {
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

vk::DescriptorSet light_buffer::get_descriptor_set() const {
    return descriptor_set_;
}

bool light_buffer::is_empty() const {
    return lights_count_ == 0;
}

uint32 light_buffer::get_lights_count() const {
    return lights_count_;
}

void light_buffer::expand_buffer_if_needed(uint32 required_count) {
    if (required_count <= capacity_) {
        return;
    }

    while (capacity_ < required_count) {
        capacity_ *= 2;
    }

    // No copy: update() rewrites the whole buffer right after this returns.
    auto new_lights_buffer = std::make_unique<storage_buffer>(
        *context_, capacity_ * sizeof(point_light_data)
    );

    deletion_->retire(std::exchange(lights_buffer_, std::move(new_lights_buffer)));
    update_descriptor_set();
}

void light_buffer::update_descriptor_set() {
    const vk::DescriptorBufferInfo storage_buffer_info{
        .buffer = lights_buffer_->get_buffer(),
        .offset = 0,
        .range  = vk::WholeSize,
    };

    context_->get_device().updateDescriptorSets(
        vk::WriteDescriptorSet{
            .dstSet          = descriptor_set_,
            .dstBinding      = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo     = &storage_buffer_info,
        },
        nullptr
    );
}

}  // namespace vw::gfx
