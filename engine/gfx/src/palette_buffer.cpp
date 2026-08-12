module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

palette_buffer::palette_buffer(
    vulkan_context& context,
    vk::DescriptorPool descriptor_pool,
    vk::DescriptorSetLayout descriptor_set_layout,
    const block_registry& registry
)
    : context_(&context)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {
    std::array<uint32, 256> palette_data{};
    for (uint8 i = 0; i < std::numeric_limits<uint8>::max(); ++i) {
        const auto bid = block_id{i};
        palette_data[i] = registry.get_color(bid).value;
    }

    buffer_ = std::make_unique<storage_buffer>(*context_, sizeof(palette_data));
    buffer_->copy_from(palette_data.data(), sizeof(palette_data));

    descriptor_set_ = vk_must(
        context_->get_device().allocateDescriptorSets({
            .descriptorPool     = descriptor_pool_,
            .descriptorSetCount = 1,
            .pSetLayouts        = &descriptor_set_layout_,
        }),
        "allocate palette descriptor set"
    ).front();

    const vk::DescriptorBufferInfo buffer_info{
        .buffer = buffer_->get_buffer(),
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
            .pBufferInfo     = &buffer_info,
        },
        nullptr
    );
}

palette_buffer::~palette_buffer() {
    if (descriptor_set_ && descriptor_pool_) {
        context_->get_device().freeDescriptorSets(descriptor_pool_, descriptor_set_);
    }
}

auto palette_buffer::get_descriptor_set() const -> vk::DescriptorSet {
    return descriptor_set_;
}

}  // namespace vw::gfx
