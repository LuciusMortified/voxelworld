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

void light_buffer::update(
    world_type& world, const spatial::frustum& frustum, const vec3f& eye
) {
    std::vector<point_light_data> point_lights_data;

    auto view = world.view<light_component, transform_component>();
    for (const auto& [ent, light_comp, transform_comp] : view) {
        const vec3f& pos      = transform_comp.get_position();
        const float32 range   = light_comp.get_range();

        // The reach is a Manhattan diamond, so the box that holds it is the
        // cube of the same half-extent -- |dx|+|dy|+|dz| <= range puts every
        // axis inside range on its own.
        const spatial::aabb reach{
            .min = vec3f{pos.x - range, pos.y - range, pos.z - range},
            .max = vec3f{pos.x + range, pos.y + range, pos.z + range},
        };

        if (!frustum.intersects(reach)) {
            continue;
        }

        const vec3f& col = light_comp.get_color();

        point_lights_data.push_back(point_light_data{
            .position  = vec4f(pos.x, pos.y, pos.z, 0.0f),
            .color     = vec4f(col.x, col.y, col.z, 0.0f),
            .intensity = light_comp.get_intensity(),
            .range     = range,
        });
    }

    // Over the cap, keep the nearest and drop the rest. Cutting the list where
    // the iteration happened to end would have dropped whichever lights the
    // ECS listed last, and the one standing next to the camera is as likely to
    // be there as any other.
    //
    // A cap and not a longer buffer: past a couple of dozen it is the per-pixel
    // loop that hurts, and more entries only feed it. The fix at that point is
    // clustering, and this is the line that marks where it would be needed.
    if (point_lights_data.size() > max_visible_) {
        const auto nth = point_lights_data.begin() + static_cast<std::ptrdiff_t>(max_visible_);

        std::nth_element(
            point_lights_data.begin(), nth, point_lights_data.end(),
            [&eye](const point_light_data& a, const point_light_data& b) -> bool {
                const auto d2 = [&eye](const point_light_data& l) -> float32 {
                    const float32 dx = l.position.x - eye.x;
                    const float32 dy = l.position.y - eye.y;
                    const float32 dz = l.position.z - eye.z;
                    return (dx * dx) + (dy * dy) + (dz * dz);
                };
                return d2(a) < d2(b);
            }
        );

        point_lights_data.resize(max_visible_);
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
