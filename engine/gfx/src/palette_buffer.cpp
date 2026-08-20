module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

namespace {

// A plain power, and not the 2.4 of the real sRGB curve. Two things are true
// here and they pull opposite ways.
//
// The shader multiplies albedo into light and light is linear, so strictly the
// palette wants decoding: the byte over 255 straight into that multiply
// overstates every colour, and the sun then pushes the product past one.
//
// But Apollo was picked by eye, on a screen, as finished pixels. Decoding it
// properly darkens it a great deal and unevenly -- putting the dark green
// 0x25562e back where it was drawn takes eight times the light, while the
// near-white 0xebede9 wants a tenth more. Nothing covers a spread of seven, so
// "correct" and "looks like the palette" are two settings, not one.
//
// 1.5 is where that was settled, on screen. It is a compromise and it is
// supposed to look like one: the darks keep some depth without falling through
// the floor, and the lit faces stay off the ceiling. The clipping that decoding
// was half meant to fix belongs to the tone curve now, so nothing downstream
// depends on this number -- until stage 3 puts coloured lights in the sum,
// where adding them over a non-linear albedo starts to shift hue, and the
// question comes back with a picture attached.
constexpr float32 palette_gamma = 1.5f;

[[nodiscard]] auto decode(uint8 channel) -> float32 {
    return std::pow(static_cast<float32>(channel) / 255.0f, palette_gamma);
}

}  // namespace

palette_buffer::palette_buffer(
    vulkan_context& context,
    vk::DescriptorPool descriptor_pool,
    vk::DescriptorSetLayout descriptor_set_layout,
    const block_registry& registry
)
    : context_(&context)
    , descriptor_pool_(descriptor_pool)
    , descriptor_set_layout_(descriptor_set_layout) {
    // All 256, not 255: the bound used to be numeric_limits<uint8>::max(), which
    // left the last entry black, and a block that landed on it would have
    // rendered unlit rather than wrong-coloured.
    std::array<vec4f, 256> palette_data{};
    for (std::size_t i = 0; i < palette_data.size(); ++i) {
        const color clr = registry.get_color(block_id{static_cast<uint8>(i)});
        palette_data[i] =
            vec4f{decode(clr.r()), decode(clr.g()), decode(clr.b()), 1.0f};
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
