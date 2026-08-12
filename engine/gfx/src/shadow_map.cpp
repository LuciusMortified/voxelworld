module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :logging;
import :vk;

namespace vw::gfx {

shadow_map::shadow_map(
    vulkan_context& context, uint32 size
)
    : context_(&context), size_(size) {
    // Инициализируем матрицы единичными
    for (auto& matrix : light_space_matrices_) {
        matrix = math::identity_matrix();
    }
    create_shadow_map_image();
    create_sampler();
    create_render_pass();
    create_framebuffers();
}

shadow_map::~shadow_map() {
    cleanup();
}

void shadow_map::create_shadow_map_image() {
    constexpr std::array candidates = {
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD24UnormS8Uint,
    };

    vk::Format depth_format = vk::Format::eUndefined;
    for (vk::Format fmt : candidates) {
        const vk::FormatProperties props = context_->get_physical_device().getFormatProperties(fmt);
        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            depth_format = fmt;
            break;
        }
    }
    if (depth_format == vk::Format::eUndefined) {
        throw std::runtime_error("Failed to find supported depth format for shadow map!");
    }

    vk::ImageCreateInfo image_info{};
    image_info.imageType     = vk::ImageType::e2D;
    image_info.extent.width  = size_;
    image_info.extent.height = size_;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = cascade_count;
    image_info.format        = depth_format;
    image_info.tiling        = vk::ImageTiling::eOptimal;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage   = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled;
    image_info.samples = vk::SampleCountFlagBits::e1;
    image_info.sharingMode = vk::SharingMode::eExclusive;

    shadow_image_ = vk_must(context_->get_device().createImage(image_info), "create shadow map image");

    const vk::MemoryRequirements mem_requirements =
        context_->get_device().getImageMemoryRequirements(shadow_image_);

    vk::MemoryAllocateInfo alloc_info{};
    alloc_info.allocationSize = mem_requirements.size;

    const vk::PhysicalDeviceMemoryProperties mem_properties =
        context_->get_physical_device().getMemoryProperties();

    uint32 memory_type_index = std::numeric_limits<uint32>::max();
    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((mem_requirements.memoryTypeBits & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags &
             vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal) {
            memory_type_index = i;
            break;
        }
    }

    if (memory_type_index == std::numeric_limits<uint32>::max()) {
        throw std::runtime_error("Failed to find suitable memory type for shadow map!");
    }

    alloc_info.memoryTypeIndex = memory_type_index;

    shadow_image_memory_ =
        vk_must(context_->get_device().allocateMemory(alloc_info), "allocate shadow map memory");

    vk_must(
        context_->get_device().bindImageMemory(shadow_image_, shadow_image_memory_, 0),
        "bind shadow map memory"
    );

    // Создаем array view для использования в шейдерах
    vk::ImageViewCreateInfo array_view_info{};
    array_view_info.image                           = shadow_image_;
    array_view_info.viewType                        = vk::ImageViewType::e2DArray;
    array_view_info.format                          = depth_format;
    array_view_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
    array_view_info.subresourceRange.baseMipLevel   = 0;
    array_view_info.subresourceRange.levelCount     = 1;
    array_view_info.subresourceRange.baseArrayLayer = 0;
    array_view_info.subresourceRange.layerCount     = cascade_count;

    shadow_array_image_view_ = vk_must(
        context_->get_device().createImageView(array_view_info), "create shadow map array view"
    );

    // Создаем отдельные 2D views для каждого каскада (для debug отображения и framebuffers)
    for (uint32 i = 0; i < cascade_count; ++i) {
        vk::ImageViewCreateInfo view_info{};
        view_info.image                           = shadow_image_;
        view_info.viewType                        = vk::ImageViewType::e2D;
        view_info.format                          = depth_format;
        view_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eDepth;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = i;
        view_info.subresourceRange.layerCount     = 1;

        shadow_cascade_image_views_[i] = vk_must(
            context_->get_device().createImageView(view_info), "create shadow map cascade view"
        );
    }
}

void shadow_map::create_sampler() {
    vk::SamplerCreateInfo sampler_info{};
    sampler_info.magFilter               = vk::Filter::eNearest;
    sampler_info.minFilter               = vk::Filter::eNearest;
    sampler_info.mipmapMode              = vk::SamplerMipmapMode::eNearest;
    sampler_info.addressModeU            = vk::SamplerAddressMode::eClampToBorder;
    sampler_info.addressModeV            = vk::SamplerAddressMode::eClampToBorder;
    sampler_info.addressModeW            = vk::SamplerAddressMode::eClampToBorder;
    sampler_info.mipLodBias              = 0.0f;
    sampler_info.anisotropyEnable        = vk::False;
    sampler_info.maxAnisotropy           = 1.0f;
    sampler_info.compareEnable           = vk::True;
    sampler_info.compareOp               = vk::CompareOp::eLessOrEqual;
    sampler_info.minLod                  = 0.0f;
    sampler_info.maxLod                  = 1.0f;
    sampler_info.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
    sampler_info.unnormalizedCoordinates = vk::False;

    shadow_sampler_ =
        vk_must(context_->get_device().createSampler(sampler_info), "create shadow map sampler");

    sampler_info.compareEnable = vk::False;
    sampler_info.compareOp     = vk::CompareOp::eNever;

    debug_sampler_ =
        vk_must(context_->get_device().createSampler(sampler_info), "create shadow debug sampler");
}

void shadow_map::create_render_pass() {
    vk::AttachmentDescription depth_attachment{};
    depth_attachment.format         = vk::Format::eD32Sfloat;
    depth_attachment.samples        = vk::SampleCountFlagBits::e1;
    depth_attachment.loadOp         = vk::AttachmentLoadOp::eClear;
    depth_attachment.storeOp        = vk::AttachmentStoreOp::eStore;
    depth_attachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.initialLayout  = vk::ImageLayout::eUndefined;
    depth_attachment.finalLayout    = vk::ImageLayout::eDepthStencilReadOnlyOptimal;

    vk::AttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 0;
    depth_attachment_ref.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount    = 0;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    vk::SubpassDependency dependency{};
    dependency.srcSubpass    = vk::SubpassExternal;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = vk::PipelineStageFlagBits::eFragmentShader;
    dependency.srcAccessMask = vk::AccessFlagBits::eShaderRead;
    dependency.dstStageMask  = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    vk::RenderPassCreateInfo render_pass_info{};
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &depth_attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses      = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies   = &dependency;

    shadow_render_pass_ = vk_must(
        context_->get_device().createRenderPass(render_pass_info), "create shadow map render pass"
    );
}

void shadow_map::create_framebuffers() {
    for (uint32 i = 0; i < cascade_count; ++i) {
        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.renderPass      = shadow_render_pass_;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments    = &shadow_cascade_image_views_[i];
        framebuffer_info.width           = size_;
        framebuffer_info.height          = size_;
        framebuffer_info.layers          = 1;

        shadow_framebuffers_[i] = vk_must(
            context_->get_device().createFramebuffer(framebuffer_info),
            "create shadow map framebuffer"
        );
    }
}

void shadow_map::update(
    const camera& camera, const vec3f& light_direction
) {
    const vec3f light_dir = math::normalize(light_direction);

    // Вычисляем split distances используя Practical Split Scheme
    const float cam_near    = camera.get_near();
    const float cam_far     = camera.get_far();
    const float shadow_far  = std::min(shadow_far_, cam_far);
    const float shadow_dist = shadow_far - cam_near;

    for (uint32 i = 0; i < cascade_count; ++i) {
        float p            = static_cast<float>(i + 1) / static_cast<float>(cascade_count);
        float log          = cam_near * std::pow(shadow_far / cam_near, p);
        float uniform      = cam_near + (shadow_dist * p);
        float d            = (split_lambda_ * (log - uniform)) + uniform;
        cascade_splits_[i] = d - cam_near;
    }

    // Вычисляем inverse view-projection матрицу камеры
    auto cam_proj =
        math::perspective_matrix(camera.get_fov(), camera.get_aspect_ratio(), cam_near, shadow_far);
    auto cam_view   = camera.get_view_matrix();
    auto inv_result = math::inverse_matrix(cam_proj * cam_view);
    auto inv_cam    = inv_result.value_or(math::identity_matrix());

    // Для каждого каскада вычисляем light space matrix
    float last_cascade_split = 0.0f;
    for (uint32 cascade_index = 0; cascade_index < cascade_count; ++cascade_index) {
        const float cascade_split = cascade_splits_[cascade_index];

        std::array frustum_corners = {
            vec3f{-1.0f, 1.0f, 0.0f},
            vec3f{1.0f, 1.0f, 0.0f},
            vec3f{1.0f, -1.0f, 0.0f},
            vec3f{-1.0f, -1.0f, 0.0f},
            vec3f{-1.0f, 1.0f, 1.0f},
            vec3f{1.0f, 1.0f, 1.0f},
            vec3f{1.0f, -1.0f, 1.0f},
            vec3f{-1.0f, -1.0f, 1.0f},
        };

        for (auto& corner : frustum_corners) {
            auto corner_homogeneous = vec4f{corner.x, corner.y, corner.z, 1.0f};
            auto corner_world       = inv_cam * corner_homogeneous;
            corner_world            = corner_world * (1.f / corner_world.w);
            corner                  = vec3f{corner_world.x, corner_world.y, corner_world.z};
        }

        for (int i = 0; i < 4; ++i) {
            vec3 dist              = frustum_corners[i + 4] - frustum_corners[i];
            frustum_corners[i + 4] = frustum_corners[i] + dist * (cascade_split / shadow_dist);
            frustum_corners[i]     = frustum_corners[i] + dist * (last_cascade_split / shadow_dist);
        }

        vec3 frustum_center = vec3f{0.0f, 0.0f, 0.0f};
        for (const auto& corner : frustum_corners) {
            frustum_center += corner;
        }
        frustum_center = frustum_center * (1.0f / static_cast<float>(frustum_corners.size()));

        float radius = 0.0f;
        for (const auto& corner : frustum_corners) {
            float distance = math::length(corner - frustum_center);
            radius         = std::max(radius, distance);
        }

        const vec3f max_extents = vec3f{radius, radius, radius};
        const vec3f min_extents = -max_extents;

        vec3f target = frustum_center;
        vec3f eye    = target - light_dir * shadow_dist;

        auto up = vec3f{0.0f, 1.0f, 0.0f};
        if (std::abs(math::dot(up, light_dir)) > 0.99f) {
            up = vec3f{1.0f, 0.0f, 0.0f};
        }

        const mat4f light_view = math::look_at_matrix(eye, target, up);

        float min_z = std::numeric_limits<float>::max();
        float max_z = std::numeric_limits<float>::lowest();
        for (const auto& corner : frustum_corners) {
            auto lv = light_view * vec4f{corner.x, corner.y, corner.z, 1.0f};
            min_z   = std::min(min_z, lv.z);
            max_z   = std::max(max_z, lv.z);
        }

        float ortho_near = std::max(-max_z - shadow_dist, 0.001f);
        float ortho_far  = -min_z;

        auto light_proj = math::orthographic_matrix(
            min_extents.x, max_extents.x, min_extents.y, max_extents.y, ortho_near, ortho_far
        );

        auto lsm = light_proj * light_view;

        float half_size     = static_cast<float>(size_) * 0.5f;
        vec4f shadow_origin = lsm * vec4f{0.0f, 0.0f, 0.0f, 1.0f};
        float rounded_x     = std::round(shadow_origin.x * half_size);
        float rounded_y     = std::round(shadow_origin.y * half_size);
        lsm[0, 3] += (rounded_x - shadow_origin.x * half_size) / half_size;
        lsm[1, 3] += (rounded_y - shadow_origin.y * half_size) / half_size;

        light_space_matrices_[cascade_index] = lsm;
        cascade_frustums_[cascade_index]    = vw::spatial::frustum::from_view_projection_matrix(lsm);

#if 0
        std::string matrix_values;
        matrix_values += "\n";
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                matrix_values += std::to_string(lsm[row, col]) + " ";
            }
            matrix_values += "\n";
        }
        log::info("{}: {}", cascade_index, matrix_values);
#endif

        last_cascade_split = cascade_split;
    }
}

mat4f shadow_map::get_light_space_matrix(
    uint32 cascade_index
) const {
    return light_space_matrices_[cascade_index];
}

auto shadow_map::
    get_light_space_matrices() const -> const std::array<mat4f, cascade_count>& {
    return light_space_matrices_;
}

auto shadow_map::get_cascade_splits() const -> const std::array<float, cascade_count>& {
    return cascade_splits_;
}

auto shadow_map::get_cascade_frustums() const -> const std::array<vw::spatial::frustum, cascade_count>& {
    return cascade_frustums_;
}

vk::Image shadow_map::get_image() const {
    return shadow_image_;
}

vk::ImageView shadow_map::get_image_view(
    uint32 cascade_index
) const {
    return shadow_cascade_image_views_[cascade_index];
}

vk::ImageView shadow_map::get_array_image_view() const {
    return shadow_array_image_view_;
}

vk::Sampler shadow_map::get_sampler() const {
    return shadow_sampler_;
}

vk::Sampler shadow_map::get_debug_sampler() const {
    return debug_sampler_;
}

vk::Framebuffer shadow_map::get_framebuffer(
    uint32 cascade_index
) const {
    return shadow_framebuffers_[cascade_index];
}

vk::RenderPass shadow_map::get_render_pass() const {
    return shadow_render_pass_;
}

void shadow_map::cleanup() {
    const vk::Device device = context_->get_device();

    for (uint32 i = 0; i < cascade_count; ++i) {
        device.destroyFramebuffer(shadow_framebuffers_[i]);
        shadow_framebuffers_[i] = nullptr;
        device.destroyImageView(shadow_cascade_image_views_[i]);
        shadow_cascade_image_views_[i] = nullptr;
    }
    device.destroyImageView(shadow_array_image_view_);
    shadow_array_image_view_ = nullptr;
    device.destroyRenderPass(shadow_render_pass_);
    shadow_render_pass_ = nullptr;
    device.destroySampler(shadow_sampler_);
    shadow_sampler_ = nullptr;
    device.destroySampler(debug_sampler_);
    debug_sampler_ = nullptr;
    device.destroyImage(shadow_image_);
    shadow_image_ = nullptr;
    device.freeMemory(shadow_image_memory_);
    shadow_image_memory_ = nullptr;
}

}  // namespace vw::gfx
